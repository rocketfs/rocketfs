// Copyright 2025 RocketFS

#pragma once

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
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace rocketfs {

class FuncStatusReturnAnalyzer {
 public:
  FuncStatusReturnAnalyzer(clang::ASTContext* ast_context,
                           clang::FunctionDecl* function_decl);

  void Visit();

 private:
  void VisitStatement(clang::Stmt* stmt);

  void HandleReturnStmt(clang::ReturnStmt* return_stmt);
  std::map<int64_t, std::set<std::string>> GetExcludedValuesInOrCond(
      const clang::IfStmt* if_stmt, const clang::Expr* cond_expr) const;
  std::map<int64_t, std::set<std::string>> GetIncludedValuesInAndCond(
      const clang::IfStmt* if_stmt, const clang::Expr* cond_expr) const;
  std::map<int64_t, std::set<std::string>> GetValueInCond(
      const clang::IfStmt* if_stmt, const clang::Expr* cond_expr) const;
  const clang::Stmt* GetParent(const clang::Stmt& stmt) const;
  std::set<std::string> ParseRetvalComments(
      clang::comments::FullComment* full_comment,
      const clang::comments::CommandTraits& command_traits) const;

 private:
  clang::ASTContext* ast_context_;
  clang::FunctionDecl* function_decl_;
  std::map<int64_t, std::set<std::string>> variable_values_;
  std::set<std::string> return_values_;
};

class StatusReturnAnalyzer
    : public clang::RecursiveASTVisitor<StatusReturnAnalyzer> {
 public:
  explicit StatusReturnAnalyzer(clang::ASTContext* ast_context);
  bool VisitFunctionDecl(clang::FunctionDecl* function_decl);

 private:
  clang::ASTContext* ast_context_;
};

class StatusReturnAnalyzerConsumer : public clang::ASTConsumer {
 public:
  void HandleTranslationUnit(clang::ASTContext& ast_context) override;
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
