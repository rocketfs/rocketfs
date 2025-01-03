// Copyright 2025 RocketFS

#include <clang/AST/ASTContext.h>
#include <clang/AST/Comment.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/LLVM.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
// #include <fmt/core.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <memory>

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "static_analysis/status_return_analyzer/status_return_analyzer.h"

namespace rocketfs {

bool StatusReturnAnalyzer::IsValidCondition(const clang::Expr* cond) const {
  assert(cond != nullptr);
  if (const auto* binary_op = llvm::dyn_cast<clang::BinaryOperator>(cond)) {
    // Only allow `||`.
    // Reject other binary operators (e.g., `&&`).
    if (binary_op->getOpcode() == clang::BO_LOr) {
      return IsValidCondition(binary_op->getLHS()) &&
             IsValidCondition(binary_op->getRHS());
    }
    return false;
  }
  return true;
}

void StatusReturnAnalyzer::CollectExcludedValues(const clang::Expr* cond) {
  assert(cond != nullptr);

  // Case 1: Logical OR (`||`)
  if (const auto* binary_op = llvm::dyn_cast<clang::BinaryOperator>(cond)) {
    if (binary_op->getOpcode() == clang::BO_LOr) {
      CollectExcludedValues(binary_op->getLHS());
      CollectExcludedValues(binary_op->getRHS());
    }
    return;
  }

  // Case 2: Single method call like `status.IsXXX()`
  if (const auto* call_expr = llvm::dyn_cast<clang::CallExpr>(cond)) {
    const auto* member =
        llvm::dyn_cast<clang::MemberExpr>(call_expr->getCallee());
    if (!member) {
      return;
    }

    // Extract the method name
    const std::string method_name = member->getMemberDecl()->getNameAsString();
    if (method_name.find("Is") == 0) {
      // Extract `XXX` from `IsXXX`
      const std::string excluded_value = "Status::" + method_name.substr(2);

      // Get the base expression (e.g., `status` in `status.IsXXX()`)
      const auto* base_expr = member->getBase()->IgnoreParenCasts();
      if (!base_expr) {
        return;
      }

      // Get the variable declaration referenced by the base expression
      const auto* decl_ref = llvm::dyn_cast<clang::DeclRefExpr>(base_expr);
      if (!decl_ref) {
        return;
      }

      const auto* var_decl =
          llvm::dyn_cast<clang::VarDecl>(decl_ref->getDecl());
      if (!var_decl) {
        return;
      }

      auto cond_parent = ast_context_->getParentMapContext()
                             .getParents(*cond)[0]
                             .get<clang::Stmt>();
      //   cond_parent = ast_context_->getParentMapContext()
      //                     .getParents(*cond_parent)[0]
      //                     .get<clang::Stmt>();
      while (cond_parent != nullptr &&
             llvm::isa<clang::CompoundStmt>(cond_parent)) {
        cond_parent = ast_context_->getParentMapContext()
                          .getParents(*cond_parent)[0]
                          .get<clang::Stmt>();
      }
      auto decl_parent = ast_context_->getParentMapContext()
                             .getParents(*var_decl)[0]
                             .get<clang::Stmt>();
      while (decl_parent != nullptr &&
             llvm::isa<clang::CompoundStmt>(decl_parent)) {
        decl_parent = ast_context_->getParentMapContext()
                          .getParents(*decl_parent)[0]
                          .get<clang::Stmt>();
      }
      if (cond_parent != decl_parent) {
        llvm::errs() << "cond_parent != decl_parent\n";
        llvm::errs() << "cond_parent\n";
        cond_parent->dump();
        llvm::errs() << "decl_parent\n";
        decl_parent->dump();
        return;
      }

      // Exclude the value from the variable's value range in `variable_values_`
      unsigned var_id = var_decl->getID();
      if (variable_values_.find(var_id) != variable_values_.end()) {
        variable_values_[var_id].erase(excluded_value);
        llvm::errs() << "Excluding value: " << excluded_value
                     << " for variable: " << var_decl->getNameAsString()
                     << "\n";
      }
    }
  }
}

StatusReturnAnalyzer::StatusReturnAnalyzer(clang::ASTContext* ast_context)
    : ast_context_(ast_context) {}

bool StatusReturnAnalyzer::VisitStatement(
    clang::Stmt* stmt, std::set<std::string>& return_value_range) {
  if (clang::DeclStmt* decl_stmt = llvm::dyn_cast<clang::DeclStmt>(stmt)) {
    // Process variable declarations
    for (clang::Decl* decl : decl_stmt->decls()) {
      if (clang::VarDecl* var_decl = llvm::dyn_cast<clang::VarDecl>(decl)) {
        if (var_decl->getType().getAsString() == "const Status") {
          unsigned var_id = var_decl->getID();
          var_decl_map[var_id] = var_decl;
          init_stmt_map[var_id] = decl_stmt;
          if (clang::CallExpr* init = llvm::dyn_cast<clang::CallExpr>(
                  var_decl->getInit()->IgnoreImplicit())) {
            if (clang::FunctionDecl* callee = init->getDirectCallee()) {
              clang::comments::FullComment* full_comment =
                  ast_context_->getCommentForDecl(callee, nullptr);
              if (full_comment) {
                const auto& retval_comments = ParseRetvalComments(
                    full_comment, ast_context_->getCommentCommandTraits());
                variable_values_[var_id].insert(retval_comments.begin(),
                                                retval_comments.end());
              }
            }
          } else {
            llvm::errs() << "Error: Status variable must be initialized with "
                            "a function call.\n";
            llvm::errs() << "Raw AST:\n";
            var_decl->getInit()->dump();
            llvm::errs() << "Pretty-printed initializer:\n";
            var_decl->getInit()->printPretty(
                llvm::errs(), nullptr, ast_context_->getPrintingPolicy());
            llvm::errs() << "\n";
          }
        }
      }
    }
  } else if (clang::ReturnStmt* return_stmt =
                 llvm::dyn_cast<clang::ReturnStmt>(stmt)) {
    // Process return statement
    HandleReturnStmt(return_stmt, return_value_range);
  } else {
    for (clang::Stmt* sub : stmt->children()) {
      VisitStatement(sub, return_value_range);
    }
  }
  return true;
}

bool StatusReturnAnalyzer::VisitFunctionDecl(clang::FunctionDecl* fn_decl) {
  llvm::dbgs() << "Visiting function: " << fn_decl->getQualifiedNameAsString()
               << "\n";

  clang::Stmt* body = fn_decl->getBody();
  if (!body) {
    llvm::dbgs() << "No body for function: "
                 << fn_decl->getQualifiedNameAsString() << "\n";
    return true;
  }

  std::set<std::string>
      return_value_range;  // The overall return value range for the function
  VisitStatement(body, return_value_range);

  // Print final return value range
  llvm::errs() << "Function: " << fn_decl->getQualifiedNameAsString() << "\n";
  llvm::errs() << "Return value range:\n";
  for (const auto& value : return_value_range) {
    llvm::errs() << "\t" << value << "\n";
  }

  return true;
}

// clang::Scope *
// StatusReturnAnalyzer::FindMeaningfulScope(clang::Scope *currentScope) {
//   while (currentScope) {
//     if (currentScope->isControlScope()) {
//       // Found a control structure scope (e.g., if, switch, while, for)
//       return currentScope;
//     }
//     if (currentScope->isFunctionScope()) {
//       // Stop at function scope
//       return currentScope;
//     }
//     currentScope = currentScope->getParent();
//   }
//   return nullptr; // No meaningful scope found
// }

void StatusReturnAnalyzer::HandleReturnStmt(
    clang::ReturnStmt* return_stmt, std::set<std::string>& return_value_range) {
  if (!return_stmt) {
    return;
  }

  const clang::Expr* ret_value = return_stmt->getRetValue()->IgnoreImplicit();
  if (!ret_value) {
    return;
  }

  const clang::Stmt* parent = ast_context_->getParentMapContext()
                                  .getParents(*return_stmt)[0]
                                  .get<clang::Stmt>();
  while (parent != nullptr && llvm::isa<clang::CompoundStmt>(parent)) {
    parent = ast_context_->getParentMapContext()
                 .getParents(*parent)[0]
                 .get<clang::Stmt>();
  }
  if (parent != nullptr) {
    // llvm::errs() << "parent's Raw AST:\n";
    // parent->dump();
    const auto* if_stmt = llvm::dyn_cast<clang::IfStmt>(parent);
    if (if_stmt) {
      const clang::Expr* cond = if_stmt->getCond();
      if (IsValidCondition(cond)) {
        std::set<std::string> excluded_values;
        const clang::Expr* status_expr = nullptr;
        CollectExcludedValues(cond);
        llvm::errs() << "in if stmt\n";
      }
    }
  }

  if (const auto* call = llvm::dyn_cast<clang::CallExpr>(ret_value)) {
    if (const clang::FunctionDecl* callee =
            call->getDirectCallee()) {  // Use const
                                        // Direct return: return Status::OK()
      if (callee->getQualifiedNameAsString().find("Status::") == 0) {
        // Old way: just use the callee's name to identify the return type
        // llvm::errs() << "returning Status type directly from: "
        //              << callee->getNameAsString() << "\n";
        return_value_range.insert("Status::" + callee->getNameAsString());
      } else {
        // Analyze the comments associated with the callee function
        // llvm::errs() << "Analyzing function call to: "
        //              << callee->getNameAsString() << "\n";
        const auto& retval_comments = ParseRetvalComments(
            ast_context_->getCommentForDecl(callee, nullptr),
            ast_context_->getCommentCommandTraits());
        return_value_range.insert(retval_comments.begin(),
                                  retval_comments.end());
      }
      // return_value_range.insert(callee->getNameAsString());
    }
  }
  //   else if (const auto *var_ref =
  //                  llvm::dyn_cast<clang::DeclRefExpr>(ret_value)) {
  //     // Variable return: return s
  //     if (const clang::VarDecl *var_decl = // Use const
  //         llvm::dyn_cast<clang::VarDecl>(var_ref->getDecl())) {
  //       unsigned var_id = var_decl->getID();
  //       AddVariableValuesToRange(var_id, return_value_range);
  //     }
  //   }
  // Handle CXXConstructExpr for return statements
  else if (const auto* construct =
               llvm::dyn_cast<clang::CXXConstructExpr>(ret_value)) {
    // Unwrap the first argument to the constructor
    if (construct->getNumArgs() > 0) {
      const clang::Expr* constructor_arg =
          construct->getArg(0)->IgnoreImplicit();
      // Check if the constructor argument is a variable reference
      if (const auto* var_ref =
              llvm::dyn_cast<clang::DeclRefExpr>(constructor_arg)) {
        if (const clang::VarDecl* var_decl =
                llvm::dyn_cast<clang::VarDecl>(var_ref->getDecl())) {
          // Variable return: return s;
          unsigned var_id = var_decl->getID();
          AddVariableValuesToRange(var_id, return_value_range);
        }
      }
    }
  } else {
    llvm::errs() << "unknown return stmt\n";
    llvm::errs() << "Raw AST:\n";
    ret_value->dump();
  }
}

std::set<std::string> StatusReturnAnalyzer::ParseRetvalComments(
    clang::comments::FullComment* full_comment,
    const clang::comments::CommandTraits& command_traits) const {
  std::set<std::string> retvals;

  if (!full_comment) {
    return retvals;
  }

  for (const auto* block : full_comment->getBlocks()) {
    if (const auto* block_command =
            llvm::dyn_cast<clang::comments::BlockCommandComment>(block)) {
      if (block_command->getCommandName(command_traits) == "retval" &&
          block_command->getNumArgs() > 0) {
        auto retval = block_command->getArgText(0).str();
        // Replace starts_with and ends_with with C++17-compatible logic
        if (retval.compare(0, 6, "Status") == 0 && retval.size() >= 2 &&
            retval.substr(retval.size() - 2) == "()") {
          retvals.emplace(retval.substr(0, retval.size() - 2));
        }
      }
    }
  }
  return retvals;
}

void StatusReturnAnalyzer::AddVariableValuesToRange(
    unsigned var_id, std::set<std::string>& return_value_range) {
  auto it = variable_values_.find(var_id);
  if (it != variable_values_.end()) {
    return_value_range.insert(it->second.begin(), it->second.end());
  }
}

StatusReturnAnalyzerConsumer::StatusReturnAnalyzerConsumer(
    const clang::CompilerInstance& compiler_instance)
    : compiler_instance_(compiler_instance) {}

void StatusReturnAnalyzerConsumer::HandleTranslationUnit(
    clang::ASTContext& ast_context) {
  StatusReturnAnalyzer analyzer(&ast_context);
  analyzer.TraverseDecl(ast_context.getTranslationUnitDecl());
}

bool StatusReturnAnalyzerAction::ParseArgs(
    const clang::CompilerInstance& compiler_instance,
    const std::vector<std::string>& args) {
  return true;
}

std::unique_ptr<clang::ASTConsumer>
StatusReturnAnalyzerAction::CreateASTConsumer(
    clang::CompilerInstance& compiler_instance, clang::StringRef in_file) {
  return std::make_unique<StatusReturnAnalyzerConsumer>(compiler_instance);
}

}  // namespace rocketfs

static clang::FrontendPluginRegistry::Add<rocketfs::StatusReturnAnalyzerAction>
    X("status-return-analyzer",
      "Analyze Status return values and their \\retval comments.");
