// Copyright 2025 RocketFS

#include "static_analysis/status_return_analyzer/status_return_analyzer.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Comment.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/LLVM.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <memory>
#include <regex>

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

namespace rocketfs {

FuncStatusReturnAnalyzer::FuncStatusReturnAnalyzer(
    clang::ASTContext* ast_context, clang::FunctionDecl* function_decl)
    : ast_context_(ast_context), function_decl_(function_decl) {
  assert(ast_context_ != nullptr);
  assert(function_decl_ != nullptr);
}

void FuncStatusReturnAnalyzer::Visit() {
  clang::Stmt* body = function_decl_->getBody();
  if (!body) {
    return;
  }
  VisitStatement(body);
  llvm::errs() << "Function: " << function_decl_->getQualifiedNameAsString()
               << "\n";
  llvm::errs() << "Return value range:\n";
  for (const auto& value : return_values_) {
    llvm::errs() << "\t" << value << "\n";
  }
}

void FuncStatusReturnAnalyzer::VisitStatement(clang::Stmt* stmt) {
  assert(stmt != nullptr);
  for (clang::Stmt* child_stmt : stmt->children()) {
    VisitStatement(child_stmt);
  }
  if (clang::DeclStmt* decl_stmt = llvm::dyn_cast<clang::DeclStmt>(stmt)) {
    for (clang::Decl* decl : decl_stmt->decls()) {
      if (clang::VarDecl* var_decl = llvm::dyn_cast<clang::VarDecl>(decl)) {
        if (var_decl->getType().getAsString() == "const Status") {
          unsigned var_id = var_decl->getID();
          clang::CallExpr* init = llvm::dyn_cast<clang::CallExpr>(
              var_decl->getInit()->IgnoreImplicit());
          assert(init != nullptr);
          clang::FunctionDecl* callee = init->getDirectCallee();
          assert(callee != nullptr);
          clang::comments::FullComment* full_comment =
              ast_context_->getCommentForDecl(callee, nullptr);
          assert(full_comment != nullptr);
          const auto& retval_comments = ParseRetvalComments(
              full_comment, ast_context_->getCommentCommandTraits());
          variable_values_[var_id].insert(retval_comments.begin(),
                                          retval_comments.end());
        }
      }
    }
  }
  if (clang::ReturnStmt* return_stmt =
          llvm::dyn_cast<clang::ReturnStmt>(stmt)) {
    HandleReturnStmt(return_stmt);
  }
}

void FuncStatusReturnAnalyzer::HandleReturnStmt(
    clang::ReturnStmt* return_stmt) {
  assert(return_stmt != nullptr);
  const clang::Expr* return_value =
      return_stmt->getRetValue()->IgnoreImplicit();
  assert(return_value != nullptr);

  const clang::Stmt* parent = ast_context_->getParentMapContext()
                                  .getParents(*return_stmt)[0]
                                  .get<clang::Stmt>();
  while (parent != nullptr && llvm::isa<clang::CompoundStmt>(parent)) {
    parent = ast_context_->getParentMapContext()
                 .getParents(*parent)[0]
                 .get<clang::Stmt>();
  }
  if (parent != nullptr) {
    const auto* if_stmt = llvm::dyn_cast<clang::IfStmt>(parent);
    if (if_stmt) {
      const clang::Expr* cond_expr = if_stmt->getCond();
      if (IsValidOrCond(cond_expr)) {
        std::set<std::string> excluded_values;
        const clang::Expr* status_expr = nullptr;
        ExcludeValuesInOrCond(cond_expr);
      }
    }
  }

  if (const auto* call = llvm::dyn_cast<clang::CallExpr>(return_value)) {
    if (const clang::FunctionDecl* callee = call->getDirectCallee()) {
      if (callee->getQualifiedNameAsString().find("Status::") == 0) {
        return_values_.insert("Status::" + callee->getNameAsString());
      } else {
        const auto& retval_comments = ParseRetvalComments(
            ast_context_->getCommentForDecl(callee, nullptr),
            ast_context_->getCommentCommandTraits());
        return_values_.insert(retval_comments.begin(), retval_comments.end());
      }
    }
  } else if (const auto* construct =
                 llvm::dyn_cast<clang::CXXConstructExpr>(return_value)) {
    // Unwrap the first argument to the constructor
    if (construct->getNumArgs() > 0) {
      const clang::Expr* constructor_arg =
          construct->getArg(0)->IgnoreImplicit();
      // Check if the constructor argument is a variable reference
      if (const auto* var_ref =
              llvm::dyn_cast<clang::DeclRefExpr>(constructor_arg)) {
        if (const clang::VarDecl* var_decl =
                llvm::dyn_cast<clang::VarDecl>(var_ref->getDecl())) {
          unsigned var_id = var_decl->getID();
          assert(variable_values_.find(var_id) != variable_values_.end());
          return_values_.insert(variable_values_[var_id].begin(),
                                variable_values_[var_id].end());
        }
      }
    }
  } else {
    assert(false);
  }
}

void FuncStatusReturnAnalyzer::ExcludeValuesInOrCond(
    const clang::Expr* cond_expr) {
  assert(cond_expr != nullptr);
  if (const auto* binary_op =
          llvm::dyn_cast<clang::BinaryOperator>(cond_expr)) {
    if (binary_op->getOpcode() == clang::BO_LOr) {
      ExcludeValuesInOrCond(binary_op->getLHS());
      ExcludeValuesInOrCond(binary_op->getRHS());
    }
    return;
  }

  // Single method call like `status.IsXXX()`.
  if (const auto* call_expr = llvm::dyn_cast<clang::CallExpr>(cond_expr)) {
    if (const auto* member =
            llvm::dyn_cast<clang::MemberExpr>(call_expr->getCallee())) {
      const auto* base_expr = member->getBase()->IgnoreParenCasts();
      if (base_expr != nullptr &&
          (base_expr->getType()->getAsCXXRecordDecl() != nullptr &&
           base_expr->getType()->getAsCXXRecordDecl()->getName() == "Status") &&
          base_expr->getType().isConstQualified()) {
        const std::string method_name =
            member->getMemberDecl()->getNameAsString();
        assert(method_name.find("Is") == 0);
        const auto excluded_value = "Status::" + method_name.substr(2);

        // Get the variable declaration referenced by the base expression.
        const auto* decl_ref_expr =
            llvm::dyn_cast<clang::DeclRefExpr>(base_expr);
        assert(decl_ref_expr != nullptr);
        const auto* var_decl =
            llvm::dyn_cast<clang::VarDecl>(decl_ref_expr->getDecl());
        assert(var_decl != nullptr);

        auto cond_expr_parent = GetParent(*cond_expr);
        assert(cond_expr_parent != nullptr);
        assert(llvm::isa<clang::IfStmt>(cond_expr_parent));
        auto cond_expr_grandparent = GetParent(*cond_expr_parent);
        auto var_decl_parent = ast_context_->getParentMapContext()
                                   .getParents(*var_decl)[0]
                                   .get<clang::Stmt>();
        assert(var_decl_parent != nullptr);
        assert(llvm::isa<clang::DeclStmt>(*var_decl_parent));
        auto var_decl_grandparent = GetParent(*var_decl_parent);
        if (cond_expr_grandparent != var_decl_grandparent) {
          return;
        }
        unsigned var_id = var_decl->getID();
        if (variable_values_.find(var_id) != variable_values_.end()) {
          variable_values_[var_id].erase(excluded_value);
        }
      }
    }
  }
}

bool FuncStatusReturnAnalyzer::IsValidOrCond(
    const clang::Expr* cond_expr) const {
  assert(cond_expr != nullptr);
  if (const auto* binary_op =
          llvm::dyn_cast<clang::BinaryOperator>(cond_expr)) {
    if (binary_op->getOpcode() == clang::BO_LOr) {
      return IsValidOrCond(binary_op->getLHS()) &&
             IsValidOrCond(binary_op->getRHS());
    }
    return false;
  }
  return true;
}

const clang::Stmt* FuncStatusReturnAnalyzer::GetParent(
    const clang::Stmt& stmt) const {
  const clang::Stmt* parent_stmt = &stmt;
  do {
    auto parents = ast_context_->getParentMapContext().getParents(*parent_stmt);
    assert(!parents.empty());
    parent_stmt = parents[0].get<clang::Stmt>();
  } while (parent_stmt != nullptr &&
           llvm::isa<clang::CompoundStmt>(parent_stmt));
  return parent_stmt;
}

std::set<std::string> FuncStatusReturnAnalyzer::ParseRetvalComments(
    clang::comments::FullComment* full_comment,
    const clang::comments::CommandTraits& command_traits) const {
  if (!full_comment) {
    return {};
  }
  std::set<std::string> return_values;
  for (const auto* block : full_comment->getBlocks()) {
    if (const auto* block_command =
            llvm::dyn_cast<clang::comments::BlockCommandComment>(block)) {
      if (block_command->getCommandName(command_traits) == "retval" &&
          block_command->getNumArgs() > 0) {
        auto retval = block_command->getArgText(0).str();
        std::regex retval_regex(R"(^(Status::\S+)\(\)$)");
        std::smatch match;
        assert(std::regex_match(retval, match, retval_regex));
        assert(match.size() == 2);
        assert(return_values.emplace(match[1].str()).second);
      }
    }
  }
  return return_values;
}

StatusReturnAnalyzer::StatusReturnAnalyzer(clang::ASTContext* ast_context)
    : ast_context_(ast_context) {
  assert(ast_context_ != nullptr);
}

bool StatusReturnAnalyzer::VisitFunctionDecl(clang::FunctionDecl* func_decl) {
  assert(func_decl != nullptr);
  FuncStatusReturnAnalyzer(ast_context_, func_decl).Visit();
  return true;
}

void StatusReturnAnalyzerConsumer::HandleTranslationUnit(
    clang::ASTContext& ast_context) {
  StatusReturnAnalyzer(&ast_context)
      .TraverseDecl(ast_context.getTranslationUnitDecl());
}

bool StatusReturnAnalyzerAction::ParseArgs(
    const clang::CompilerInstance& compiler_instance,
    const std::vector<std::string>& args) {
  return true;
}

std::unique_ptr<clang::ASTConsumer>
StatusReturnAnalyzerAction::CreateASTConsumer(
    clang::CompilerInstance& compiler_instance, clang::StringRef in_file) {
  return std::make_unique<StatusReturnAnalyzerConsumer>();
}

}  // namespace rocketfs

static clang::FrontendPluginRegistry::Add<rocketfs::StatusReturnAnalyzerAction>
    X("status-return-analyzer",
      "Analyze Status return values and their \\retval comments.");
