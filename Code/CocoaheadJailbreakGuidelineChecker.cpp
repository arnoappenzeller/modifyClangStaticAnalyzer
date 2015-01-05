#include "ClangSACheckers.h"
#include "SelectorExtras.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprObjC.h"
#include "clang/AST/StmtObjC.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <iostream>

//===----------------------------------------------------------------------===//
//
// This checks for an isDeviceJailbroken after the application is marked as Intern
// via the SecurityKit which is a private SecurityFramework.
//
// The SecurityKit forces by its Guidelines that an App which is marked with an InternLevel
// to be checked for Jailbreak.
//
// The SecurityLevel is realized by an enum with different values. It's important to know
// that Intern is 1.
//
// isDeviceJailbroken is a objc-method that checks for jailbroken Devices and returns a bool
//
// It also made sure that the isDeviceJailbroken call comes from the SecurityKit
// and not another checked application.
// This realised by this Checker
//===----------------------------------------------------------------------===//

using namespace clang;
using namespace ento;

class CocoaheadJailbreakGuidlineChecker: public Checker <check::PostObjCMessage,
                                                      check::EndFunction> {

  std::unique_ptr<BugType> NoJailbreakCheckPerformedBugType;

  mutable IdentifierInfo *IISecurityKit, *IISetSecurityLevel;
  mutable Selector isJailbreakCheckS;

  void initIdentifierInfo(ASTContext &Ctx) const;


  bool isArgUnConstrained(Optional<NonLoc>, SValBuilder &, ProgramStateRef) const;
  public:
    CocoaheadJailbreakGuidlineChecker(void): IISecurityKit(0),IISetSecurityLevel(nullptr){
      NoJailbreakCheckPerformedBugType.reset(new BugType(this,
       "CocoaheadJailbreakGuidlineChecker: Application is marked as Intern, but no Jailbreak Check made in same function context",
        "GISJailbreakGuidline Error"));

    }

  void checkPostObjCMessage(const ObjCMethodCall &msg, CheckerContext &C) const;
  void checkEndFunction(CheckerContext &C) const;
};

//register properties for programstates to mark if a intern value is checked and if an jailbreak check was made
REGISTER_TRAIT_WITH_PROGRAMSTATE(InternSecurityLevel, bool) 
REGISTER_TRAIT_WITH_PROGRAMSTATE(JailbreakCheckMade, bool)

//checks every Objc Method Call
void CocoaheadJailbreakGuidlineChecker::checkPostObjCMessage(const ObjCMethodCall &msg, CheckerContext &C) const {
  
  //init necessary stuff
  ProgramStateRef state = C.getState(); //gets current state
  Selector S = msg.getSelector(); //get selector of msg call
  bool currentInternLevel = state->get<InternSecurityLevel>();

  //if InternLevel is setted check for jailbreak
  if (currentInternLevel)
  {
    //first check if class matches
    const ObjCInterfaceDecl *ID = msg.getReceiverInterface();
    if (!ID)
       return;
    if (ID->getIdentifier()->getName().compare("SecurityKit") == 0)
    {
      //class matches so check further
      ASTContext &Ctx = C.getASTContext();
      isJailbreakCheckS = GetNullarySelector("isDeviceJailbroken", Ctx);
      if (S == isJailbreakCheckS){
        state = state->set<JailbreakCheckMade>(true);
        C.addTransition(state);
      }
      return;
    }
  }
    //init identifier for setSecurityLevel - remeber that Object.property is syntactic sugar for [Object setProperty:]
  if (!IISetSecurityLevel)
    IISetSecurityLevel = &C.getASTContext().Idents.get("setSecurityLevel");
  //compare first Identifier of selector
  if(S.getIdentifierInfoForSlot(0) == IISetSecurityLevel){
      //check class to avoid errors
      const ObjCInterfaceDecl *ID = msg.getReceiverInterface();
      if (!ID)
          return;
      if (ID->getIdentifier()->getName().compare("SecurityKit") == 0)
      {
          //get argument
          const Expr *theArg = msg.getArgExpr(0);
          Expr::EvalResult result;
          
          //unfold the value of the argument
          if (theArg->EvaluateAsRValue(result, C.getASTContext())){
              APValue theVal = result.Val;
              
              //compare the arguments value - the Intern enum is 0
              if (theVal.getInt() == 0)
              {
                  //now we know that at this node the SecurityLevel is intern so we need to save it in the state
                  state = state->set<InternSecurityLevel>(true);
                  C.addTransition(state);
                  
              }
          }
      }
  }
  else{
      
  }

}

//check if jailbreak check is reached till end of function
void CocoaheadJailbreakGuidlineChecker::checkEndFunction(CheckerContext &C) const {
  ProgramStateRef state = C.getState();

  //get state values
  bool currentCheckMade = state->get<JailbreakCheckMade>();
  bool currentInternLevel = state->get<InternSecurityLevel>();


  if(!currentCheckMade && currentInternLevel) {
    ExplodedNode * bugloc = C.generateSink();
    if(bugloc) {
      BugReport *bug = new BugReport(*NoJailbreakCheckPerformedBugType, 
                            "Cocoahead Check: Reached end of function without Jailbreakcheck after marking app as Intern", 
                            bugloc);
      C.emitReport(bug);
    }
  }

}

void CocoaheadJailbreakGuidlineChecker::initIdentifierInfo(ASTContext &Ctx) const {
  IISecurityKit = &Ctx.Idents.get("SecurityKit");
}


void ento::registerCocoaheadJailbreakGuidlineChecker(CheckerManager &mgr) {
  mgr.registerChecker<CocoaheadJailbreakGuidlineChecker>();
}
