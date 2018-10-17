#include <sstream>
#include <string>
#include <iostream>
#include <fstream>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;
using namespace std;

static cl::OptionCategory ToolingSampleCategory("Tooling Sample");

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
  public:
    MyASTVisitor(Rewriter &R) : TheRewriter(R) {}

    bool VisitIfStmt(IfStmt *IfStatement) {
      // Only care about If statements.
      //if (isa<IfStmt>(s)) {
        //IfStmt *IfStatement = cast<IfStmt>(s);
        Stmt *Then = IfStatement->getThen();

        TheRewriter.InsertText(Then->getLocStart(), "\n// the 'if' part\n", true,
                               true);
        SourceLocation end = Then->getLocEnd();
        int offset = Lexer::MeasureTokenLength(end,
                                       TheRewriter.getSourceMgr(),
                                       TheRewriter.getLangOpts()) + 1;

        SourceLocation realEnd = end.getLocWithOffset(offset);
        TheRewriter.InsertText(realEnd, "\n// end of the 'if' part\n", true,
                               true);

        Stmt *Else = IfStatement->getElse();
        if (Else) {
          TheRewriter.InsertText(Else->getLocStart(), "\n// the 'else' part\n",
                                 true, true);
          end = Else->getLocEnd();
          offset = Lexer::MeasureTokenLength(end,
                                       TheRewriter.getSourceMgr(),
                                       TheRewriter.getLangOpts()) + 1;

          realEnd = end.getLocWithOffset(offset);
          TheRewriter.InsertText(realEnd, "\n// end of the 'else' part\n", true,
                               true);
        }
      //}

      return true;
    }

    bool VisitFunctionDecl(FunctionDecl *f) {
      // Only function definitions (with bodies), not declarations.
      if (f->hasBody()) {
        Stmt *FuncBody = f->getBody();

        // Type name as string
        QualType QT = f->getReturnType();
        string TypeStr = QT.getAsString();

        // Function name
        DeclarationName DeclName = f->getNameInfo().getName();
        string FuncName = DeclName.getAsString();

        // Add comment before
        stringstream SSBefore;
        SSBefore << "// Begin function " << FuncName << " returning " << TypeStr
                 << "\n";
        SourceLocation ST = f->getSourceRange().getBegin();
        TheRewriter.InsertText(ST, SSBefore.str(), true, true);

        // And after
        stringstream SSAfter;
        SSAfter << "\n// End function " << FuncName;
        ST = FuncBody->getLocEnd().getLocWithOffset(1);
        TheRewriter.InsertText(ST, SSAfter.str(), true, true);
      }

      return true;
    }

  private:
    Rewriter &TheRewriter;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
  public:
    MyASTConsumer(Rewriter &R) : Visitor(R) {}

    // Override the method that gets called for each parsed top-level
    // declaration.
    bool HandleTopLevelDecl(DeclGroupRef DR) override {
      for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
        // Traverse the declaration using our AST visitor.
        Visitor.TraverseDecl(*b);
        //(*b)->dump();
      }
      return true;
    }

  private:
    MyASTVisitor Visitor;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
  public:
    MyFrontendAction() {}
    void EndSourceFileAction() override {
      SourceManager &SM = TheRewriter.getSourceMgr();
      string file = SM.getFileEntryForID(SM.getMainFileID())->getName();
      outs() << "** EndSourceFileAction for: "
                   << file << "\n";

      // Now emit the rewritten buffer.
      file.insert(file.begin()+file.rfind('.'),'M');
      //ofstream newFile (file, ios::trunc);
      error_code error_code;
      raw_fd_ostream newFile(file, error_code, sys::fs::F_None);
      TheRewriter.getEditBuffer(SM.getMainFileID()).write(newFile);
      newFile.close();
    }

    unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                   StringRef file) override {
      outs() << "** Creating AST consumer for: " << file << "\n";
      TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
      return make_unique<MyASTConsumer>(TheRewriter);
    }

  private:
    Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, ToolingSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
