// Copyright 2025 RocketFS

#include "static_analysis/status_return_analyzer/status_return_analyzer.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Comment.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtIterator.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <stdint.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace rocketfs {

const std::string_view kRocketNamespace = "rocketfs::";
const std::string_view kRocketfsStatusType = "rocketfs::Status";
const std::string_view kRocketfsStatusScope = "rocketfs::Status::";
const std::string_view kRetvalCommand = "retval";
const std::regex kRetvalRegex(R"(^(Status::\S+)\(\)$)");

FuncStatusReturnAnalyzer::FuncStatusReturnAnalyzer(
    clang::ASTContext* ast_context, clang::FunctionDecl* function_decl)
    : ast_context_(ast_context), function_decl_(function_decl) {
  assert(ast_context_ != nullptr);
  assert(function_decl_ != nullptr);
}

void FuncStatusReturnAnalyzer::Visit() {
  clang::Stmt* body_stmt = function_decl_->getBody();
  if (!body_stmt) {
    return;
  }

  VisitStatement(body_stmt);

  clang::comments::FullComment* full_comment =
      ast_context_->getCommentForDecl(function_decl_, nullptr);
  assert(full_comment != nullptr);
  const auto& retval_comments = ParseRetvalComments(
      full_comment, ast_context_->getCommentCommandTraits());

  std::set<std::string> extra_retvals;
  std::set_difference(retval_comments.begin(),
                      retval_comments.end(),
                      return_values_.begin(),
                      return_values_.end(),
                      std::inserter(extra_retvals, extra_retvals.end()));
  std::set<std::string> missing_retvals;
  std::set_difference(return_values_.begin(),
                      return_values_.end(),
                      retval_comments.begin(),
                      retval_comments.end(),
                      std::inserter(missing_retvals, missing_retvals.end()));

  if (extra_retvals.empty() && missing_retvals.empty()) {
    llvm::errs() << "Function `" << function_decl_->getQualifiedNameAsString()
                 << "`'s return value comments and actual return values are "
                    "consistent.\n";
  } else {
    llvm::errs() << "Function `" << function_decl_->getQualifiedNameAsString()
                 << "`'s not consistent\n";
  }
}

void FuncStatusReturnAnalyzer::VisitStatement(clang::Stmt* stmt) {
  assert(stmt != nullptr);
  for (clang::Stmt* child_stmt : stmt->children()) {
    if (child_stmt != nullptr) {
      VisitStatement(child_stmt);
    }
  }
  if (clang::DeclStmt* decl_stmt = llvm::dyn_cast<clang::DeclStmt>(stmt)) {
    for (clang::Decl* decl : decl_stmt->decls()) {
      if (clang::VarDecl* var_decl = llvm::dyn_cast<clang::VarDecl>(decl)) {
        if ((var_decl->getType()->getAsCXXRecordDecl() != nullptr &&
             var_decl->getType()
                     ->getAsCXXRecordDecl()
                     ->getQualifiedNameAsString() == kRocketfsStatusType) &&
            var_decl->getType().isConstQualified()) {
          int64_t var_id = var_decl->getID();
          clang::CallExpr* init = llvm::dyn_cast<clang::CallExpr>(
              var_decl->getInit()->IgnoreImplicit());
          assert(init != nullptr);
          clang::FunctionDecl* callee_func_decl = init->getDirectCallee();
          assert(callee_func_decl != nullptr);
          clang::comments::FullComment* full_comment =
              ast_context_->getCommentForDecl(callee_func_decl, nullptr);
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
  const clang::Expr* return_expr = return_stmt->getRetValue()->IgnoreImplicit();
  assert(return_expr != nullptr);

  std::optional<int64_t> if_var_id;
  std::set<std::string> if_excluded_values;
  std::set<std::string> if_included_values;
  if (const clang::Stmt* parent_stmt = GetParent(*return_stmt)) {
    const auto* if_stmt = llvm::dyn_cast<clang::IfStmt>(parent_stmt);
    if (if_stmt) {
      const clang::Expr* cond_expr = if_stmt->getCond();
      auto excluded_values_in_or_cond =
          GetExcludedValuesInOrCond(if_stmt, cond_expr);
      auto included_values_in_and_cond =
          GetIncludedValuesInAndCond(if_stmt, cond_expr);
      assert(excluded_values_in_or_cond.size() <= 1);
      assert(included_values_in_and_cond.size() <= 1);
      assert(excluded_values_in_or_cond.empty() ||
             included_values_in_and_cond.empty());
      if (!excluded_values_in_or_cond.empty()) {
        if_var_id = excluded_values_in_or_cond.begin()->first;
        if_excluded_values =
            std::move(excluded_values_in_or_cond.begin()->second);
      }
      if (!included_values_in_and_cond.empty()) {
        if_var_id = included_values_in_and_cond.begin()->first;
        if_included_values =
            std::move(included_values_in_and_cond.begin()->second);
      }
    }
  }

  bool can_optimize_if_var_values = false;
  do {
    // Case 1: Directly returns a `Status` value.
    // ```cpp
    // return Status::OK();
    // ```
    if (const auto* call_expr = llvm::dyn_cast<clang::CallExpr>(return_expr)) {
      if (const clang::FunctionDecl* callee_func_decl =
              call_expr->getDirectCallee()) {
        if (callee_func_decl->getQualifiedNameAsString().find(
                kRocketfsStatusScope) == 0) {
          can_optimize_if_var_values = true;
          return_values_.insert(std::string(kRocketfsStatusScope) +
                                callee_func_decl->getNameAsString());
          break;
        }
      }
    }
    // Case 2: Returns a variable `s` that holds the result of a function call.
    // ```cpp
    // const Status s = CallingToSomeFunc();
    // return s;
    // ```
    if (const auto* construct_expr =
            llvm::dyn_cast<clang::CXXConstructExpr>(return_expr)) {
      // Unwrap the first argument to the constructor.
      if (construct_expr->getNumArgs() > 0) {
        const clang::Expr* constructor_expr =
            construct_expr->getArg(0)->IgnoreImplicit();
        // Check if the constructor argument is a variable reference.
        if (const auto* var_ref_expr =
                llvm::dyn_cast<clang::DeclRefExpr>(constructor_expr)) {
          if (const clang::VarDecl* var_decl =
                  llvm::dyn_cast<clang::VarDecl>(var_ref_expr->getDecl())) {
            int64_t var_id = var_decl->getID();
            assert(variable_values_.find(var_id) != variable_values_.end());
            if (if_var_id.has_value() && *if_var_id == var_id) {
              can_optimize_if_var_values = true;
              assert(if_excluded_values.empty() != if_included_values.empty());
              std::set<std::string> return_values;
              if (!if_excluded_values.empty()) {
                // e.g.,
                // ```cpp
                // if (s.IsInternalError() || s.IsLogicalError()) {
                //   return s;
                // }
                // ```
                std::set_intersection(
                    variable_values_[var_id].begin(),
                    variable_values_[var_id].end(),
                    if_excluded_values.begin(),
                    if_excluded_values.end(),
                    std::inserter(return_values, return_values.end()));
              } else {
                // e.g.,
                // ```cpp
                // if (!s.IsInternalError() && !s.IsLogicalError()) {
                //   return s;
                // }
                // ```
                std::set_difference(
                    variable_values_[var_id].begin(),
                    variable_values_[var_id].end(),
                    if_included_values.begin(),
                    if_included_values.end(),
                    std::inserter(return_values, return_values.end()));
              }
              return_values_.insert(return_values.begin(), return_values.end());
            } else {
              return_values_.insert(variable_values_[var_id].begin(),
                                    variable_values_[var_id].end());
            }
            break;
          }
        }
      }
    }
    // Case 3: Directly returns the result of a function call.
    // ```cpp
    // return CallingToSomeFunc();
    // ```
    if (const auto* call_expr = llvm::dyn_cast<clang::CallExpr>(return_expr)) {
      if (const clang::FunctionDecl* callee_func_decl =
              call_expr->getDirectCallee()) {
        can_optimize_if_var_values = true;
        assert(callee_func_decl->getQualifiedNameAsString().find(
                   kRocketfsStatusScope) != 0);
        const auto& retval_comments = ParseRetvalComments(
            ast_context_->getCommentForDecl(callee_func_decl, nullptr),
            ast_context_->getCommentCommandTraits());
        return_values_.insert(retval_comments.begin(), retval_comments.end());
        break;
      }
    }
    assert(false);
  } while (false);

  if (if_var_id.has_value() && can_optimize_if_var_values) {
    auto var_id = *if_var_id;
    if (!if_excluded_values.empty()) {
      assert(variable_values_.find(var_id) != variable_values_.end());
      std::set<std::string> values;
      std::set_difference(variable_values_[var_id].begin(),
                          variable_values_[var_id].end(),
                          if_excluded_values.begin(),
                          if_excluded_values.end(),
                          std::inserter(values, values.end()));
      variable_values_[var_id] = std::move(values);
    }
    if (!if_included_values.empty()) {
      assert(variable_values_.find(var_id) != variable_values_.end());
      std::set<std::string> values;
      std::set_intersection(variable_values_[var_id].begin(),
                            variable_values_[var_id].end(),
                            if_included_values.begin(),
                            if_included_values.end(),
                            std::inserter(values, values.end()));
      variable_values_[var_id] = std::move(values);
    }
  }
}

/**
 * @details
 * Optimizes the following pattern:
 * @code{.cpp}
 * const Status s = CallingToSomeFunc();
 * if (s.IsInternalError() || s.IsLogicalError()) {
 *     return Status::IOException();
 * }
 * @endcode
 *
 * In this case, the possible values of the variable `s` can be narrowed:
 * `Status::InternalError` and `Status::LogicalError` are removed from `s`'s
 * possible values.
 *
 * @note
 * This optimization is applied only when the condition expression is a single
 * variable connected with logical OR (e.g., `if (s.IsInternalError() ||
 * s.IsLogicalError())`). More complex expressions, such as `if
 * (s1.IsInternalError() || s2.IsLogicalError())` or `if (s.IsInternalError() ||
 * !s.IsLogicalError())`, are not optimized.
 *
 * @note
 * This optimization is also applied only when the return value is a
 * `Status::XXX` constant or the variable `s` itself, provided that `s` is the
 * only variable used in the condition expression. In all other cases, this
 * optimization is disabled.
 */
std::map<int64_t, std::set<std::string>>
FuncStatusReturnAnalyzer::GetExcludedValuesInOrCond(
    const clang::IfStmt* if_stmt, const clang::Expr* cond_expr) const {
  assert(cond_expr != nullptr);
  if (const auto* binary_op =
          llvm::dyn_cast<clang::BinaryOperator>(cond_expr)) {
    if (binary_op->getOpcode() == clang::BO_LOr) {
      auto lhs_excluded_values =
          GetExcludedValuesInOrCond(if_stmt, binary_op->getLHS());
      auto rhs_excluded_values =
          GetExcludedValuesInOrCond(if_stmt, binary_op->getRHS());
      assert(lhs_excluded_values.size() <= 1);
      assert(rhs_excluded_values.size() <= 1);
      if (!lhs_excluded_values.empty() && !rhs_excluded_values.empty()) {
        auto lhs_var_id = lhs_excluded_values.begin()->first;
        auto rhs_var_id = rhs_excluded_values.begin()->first;
        if (lhs_var_id == rhs_var_id) {
          lhs_excluded_values[lhs_var_id].insert(
              rhs_excluded_values[rhs_var_id].begin(),
              rhs_excluded_values[rhs_var_id].end());
          return lhs_excluded_values;
        }
      }
    }
  }

  // Single method call like `status.IsXXX()`.
  return GetValueInCond(if_stmt, cond_expr);
}

std::map<int64_t, std::set<std::string>>
FuncStatusReturnAnalyzer::GetIncludedValuesInAndCond(
    const clang::IfStmt* if_stmt, const clang::Expr* cond_expr) const {
  assert(cond_expr != nullptr);
  if (const auto* binary_op =
          llvm::dyn_cast<clang::BinaryOperator>(cond_expr)) {
    if (binary_op->getOpcode() == clang::BO_LAnd) {
      auto lhs_included_values =
          GetIncludedValuesInAndCond(if_stmt, binary_op->getLHS());
      auto rhs_included_values =
          GetIncludedValuesInAndCond(if_stmt, binary_op->getRHS());
      assert(lhs_included_values.size() <= 1);
      assert(rhs_included_values.size() <= 1);
      if (!lhs_included_values.empty() && !rhs_included_values.empty()) {
        auto lhs_var_id = lhs_included_values.begin()->first;
        auto rhs_var_id = rhs_included_values.begin()->first;
        if (lhs_var_id == rhs_var_id) {
          lhs_included_values[lhs_var_id].insert(
              rhs_included_values[rhs_var_id].begin(),
              rhs_included_values[rhs_var_id].end());
          return lhs_included_values;
        }
      }
    }
  }

  // Unary operation like `!status.IsXXX()`.
  if (const auto* unary_op = llvm::dyn_cast<clang::UnaryOperator>(cond_expr)) {
    if (unary_op->getOpcode() == clang::UO_LNot) {
      return GetValueInCond(if_stmt, unary_op->getSubExpr());
    }
  }

  return {};
}

std::map<int64_t, std::set<std::string>>
FuncStatusReturnAnalyzer::GetValueInCond(const clang::IfStmt* if_stmt,
                                         const clang::Expr* cond_expr) const {
  assert(if_stmt != nullptr);
  assert(cond_expr != nullptr);
  const clang::Stmt* cond_expr_ancestor = cond_expr;
  do {
    cond_expr_ancestor = GetParent(*cond_expr_ancestor);
  } while (cond_expr_ancestor != nullptr && cond_expr_ancestor != if_stmt);
  assert(cond_expr_ancestor == if_stmt);

  // Single method call like `status.IsXXX()`.
  if (const auto* call_expr = llvm::dyn_cast<clang::CallExpr>(cond_expr)) {
    if (const auto* member_expr =
            llvm::dyn_cast<clang::MemberExpr>(call_expr->getCallee())) {
      const clang::Expr* base_expr = member_expr->getBase()->IgnoreParenCasts();
      if (base_expr != nullptr &&
          (base_expr->getType()->getAsCXXRecordDecl() != nullptr &&
           base_expr->getType()
                   ->getAsCXXRecordDecl()
                   ->getQualifiedNameAsString() == kRocketfsStatusType) &&
          base_expr->getType().isConstQualified()) {
        const std::string method_name =
            member_expr->getMemberDecl()->getNameAsString();
        assert(method_name.find("Is") == 0);
        const auto excluded_value =
            std::string(kRocketfsStatusScope) + method_name.substr(2);

        // Get the variable declaration referenced by the base expression.
        const auto* decl_ref_expr =
            llvm::dyn_cast<clang::DeclRefExpr>(base_expr);
        assert(decl_ref_expr != nullptr);
        const auto* var_decl =
            llvm::dyn_cast<clang::VarDecl>(decl_ref_expr->getDecl());
        assert(var_decl != nullptr);

        auto if_stmt_parent = GetParent(*if_stmt);
        auto var_decl_parent = ast_context_->getParentMapContext()
                                   .getParents(*var_decl)[0]
                                   .get<clang::DeclStmt>();
        assert(var_decl_parent != nullptr);
        auto var_decl_grandparent = GetParent(*var_decl_parent);
        if (if_stmt_parent == var_decl_grandparent) {
          int64_t var_id = var_decl->getID();
          return {{var_id, {excluded_value}}};
        }
      }
    }
  }
  return {};
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
  assert(full_comment != nullptr);
  std::set<std::string> return_values;
  for (const clang::comments::BlockContentComment* block_content_comment :
       full_comment->getBlocks()) {
    if (const auto* block_command_comment =
            llvm::dyn_cast<clang::comments::BlockCommandComment>(
                block_content_comment)) {
      if (block_command_comment->getCommandName(command_traits) ==
              std::string(kRetvalCommand) &&
          block_command_comment->getNumArgs() > 0) {
        auto retval = block_command_comment->getArgText(0).str();
        std::smatch match;
        assert(std::regex_match(retval, match, kRetvalRegex));
        assert(match.size() == 2);
        assert(return_values
                   .emplace(std::string(kRocketNamespace) + match[1].str())
                   .second);
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
      "Analyze `Status` return values and their @retval comments.");
