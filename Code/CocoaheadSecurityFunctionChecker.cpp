#include "ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include <iostream>
#include <vector>

using namespace clang;
using namespace ento;

namespace {


}
class CocoaheadSecurityFunctionChecker: public Checker < check::PreCall  > {
  std::unique_ptr<BugType> boBug, ufhBug;
  mutable std::vector<IdentifierInfo*> bufferOverflowIdentifierVector, unsafeFileHandleIdentifierVector;
  mutable IdentifierInfo *IIprintf,*IIstrcat,*IIstrcpy,*IIstrncat, 
  *IIstrncpy, *IIsprintf, *IIvsprintf, *IIgets, *IIfopen, *IIchmod,
  *IIchown ,*IIstat , *IImktemp;

  void initIdentifierInfo(ASTContext &Ctx) const;


public:
  CocoaheadSecurityFunctionChecker(void): IIprintf(0),IIstrcat(0),IIstrcpy(0),IIstrncat(0),IIstrncpy(0),IIsprintf(0),IIvsprintf(0),IIgets(0) {
    boBug.reset(new BugType(this, "Potential BufferOverflow Function", "CocoaheadSecurityFunctionChecker Error"));
    ufhBug.reset(new BugType(this, "Unsafe File Handle Function", "CocoaheadSecurityFunctionChecker Error"));

  }
  void checkPreCall(const CallEvent &, CheckerContext &) const;
};

void CocoaheadSecurityFunctionChecker::checkPreCall(const CallEvent &Call, CheckerContext &C) const {
  initIdentifierInfo(C.getASTContext());
  const IdentifierInfo *ID = Call.getCalleeIdentifier();


  //check if 
  if (std::find(bufferOverflowIdentifierVector.begin(), bufferOverflowIdentifierVector.end(), ID) != bufferOverflowIdentifierVector.end()){
    ExplodedNode *N = C.addTransition(NULL);
    BugReport *bug = new BugReport(*boBug, "CocoaheadSecurityFunctionChecker: Use of a potential Buffer Overflow Function. Usage is not allowed.", N);
    C.emitReport(bug);
  }

  if (std::find(unsafeFileHandleIdentifierVector.begin(), unsafeFileHandleIdentifierVector.end(), ID) != unsafeFileHandleIdentifierVector.end()){
    ExplodedNode *ErrNode = C.generateSink();
    BugReport *bug = new BugReport(*ufhBug, "CocoaheadSecurityFunctionChecker: Use of a unsafe file handeling function. Usage is not allowed.", ErrNode);
    C.emitReport(bug);
  }

  return; 
}

void CocoaheadSecurityFunctionChecker::initIdentifierInfo(ASTContext &Ctx) const {
  //gis req 15  bufferoverflow
  IIprintf = &Ctx.Idents.get("printf");
  IIstrcat = &Ctx.Idents.get("strcat");
  IIstrcpy = &Ctx.Idents.get("strcpy");
  IIstrncat = &Ctx.Idents.get("strncat");
  IIstrncpy = &Ctx.Idents.get("strncpy");
  IIsprintf = &Ctx.Idents.get("sprintf");
  IIvsprintf = &Ctx.Idents.get("vsprintf");
  IIgets = &Ctx.Idents.get("gets");
  
  //add them to a vector
  bufferOverflowIdentifierVector.push_back(IIprintf);
  bufferOverflowIdentifierVector.push_back(IIstrcat);
  bufferOverflowIdentifierVector.push_back(IIstrcpy);
  bufferOverflowIdentifierVector.push_back(IIstrncat);
  bufferOverflowIdentifierVector.push_back(IIstrncpy);
  bufferOverflowIdentifierVector.push_back(IIsprintf);
  bufferOverflowIdentifierVector.push_back(IIvsprintf);
  bufferOverflowIdentifierVector.push_back(IIgets);

  
  IIfopen = &Ctx.Idents.get("fopen");
  IIchmod = &Ctx.Idents.get("chmod");
  IIchown = &Ctx.Idents.get("chown");
  IIstat = &Ctx.Idents.get("stat");
  IImktemp = &Ctx.Idents.get("mktemp");

  unsafeFileHandleIdentifierVector.push_back(IIfopen);
  unsafeFileHandleIdentifierVector.push_back(IIchmod);
  unsafeFileHandleIdentifierVector.push_back(IIchown);
  unsafeFileHandleIdentifierVector.push_back(IIstat);
  unsafeFileHandleIdentifierVector.push_back(IImktemp);
}


void ento::registerCocoaheadSecurityFunctionChecker(CheckerManager &mgr) {
  mgr.registerChecker<CocoaheadSecurityFunctionChecker>();
}
