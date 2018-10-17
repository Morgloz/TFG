#include <sstream>
#include <string>
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;
using namespace std;

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
  public:
    MyASTVisitor(Rewriter &R) : TheRewriter(R) {}
  private:
    Rewriter &TheRewriter;
};

class MyASTConsumer : public ASTConsumer {
  public:
    MyASTConsumer(Rewriter &R) : Visitor(R) {}

    bool HandleTopLevelDecl(DeclGroupRef DR) override {
      for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
        (*b)->dump();
      }
      return true;
    }
  private:
    MyASTVisitor Visitor;
};

class MyFrontendAction : public ASTFrontendAction {
  public:
    MyFrontendAction() {}
    unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
      outs() << "** Creating AST consumer for: " << file << "\n";
      TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
      return make_unique<MyASTConsumer>(TheRewriter);
    }
  private:
    Rewriter TheRewriter;
};

static cl::OptionCategory ToolingSampleCategory("Tooling Sample");

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, ToolingSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());
  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
