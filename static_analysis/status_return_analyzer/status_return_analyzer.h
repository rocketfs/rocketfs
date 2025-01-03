// Copyright 2025 RocketFS

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Sema/Scope.h>
#include <clang/Sema/Sema.h>
#include <llvm/Support/raw_ostream.h>

#include <map>
#include <set>
#include <string>
#include <unordered_map>

namespace rocketfs {

class StatusReturnAnalyzer
    : public clang::RecursiveASTVisitor<StatusReturnAnalyzer> {
 public:
  explicit StatusReturnAnalyzer(clang::ASTContext* ast_context);

  bool VisitFunctionDecl(clang::FunctionDecl* function_decl);

 private:
  std::set<std::string> ParseRetvalComments(
      clang::comments::FullComment* full_comment,
      const clang::comments::CommandTraits& command_traits) const;
  void HandleReturnStmt(clang::ReturnStmt* return_stmt,
                        std::set<std::string>& return_value_range);
  void AddVariableValuesToRange(unsigned var_id,
                                std::set<std::string>& return_value_range);

  bool IsValidCondition(const clang::Expr* cond) const;
  bool HasSingleAllowedBranchBetween(const clang::Stmt* decl_stmt,
                                     const clang::Stmt* return_stmt,
                                     clang::ASTContext& context,
                                     const clang::VarDecl* var_decl) const;
  void CollectExcludedValues(const clang::Expr* cond);

  clang::Scope* FindMeaningfulScope(clang::Scope* currentScope);
  bool VisitStatement(clang::Stmt* body,
                      std::set<std::string>& return_value_range);

 private:
  clang::ASTContext* ast_context_;
  std::map<unsigned, std::set<std::string>> variable_values_;
  std::map<unsigned, const clang::VarDecl*> var_decl_map;
  std::map<unsigned, const clang::Stmt*> init_stmt_map;
};

class StatusReturnAnalyzerConsumer : public clang::ASTConsumer {
 public:
  StatusReturnAnalyzerConsumer(
      const clang::CompilerInstance& compiler_instance);

  void HandleTranslationUnit(clang::ASTContext& ast_context) override;

 private:
  const clang::CompilerInstance& compiler_instance_;
};

class StatusReturnAnalyzerAction : public clang::PluginASTAction {
 public:
  bool ParseArgs(const clang::CompilerInstance& compiler_instance,
                 const std::vector<std::string>& args) override;
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance& compiler_instance,
      clang::StringRef in_file) override;
};

}  // namespace rocketfs
