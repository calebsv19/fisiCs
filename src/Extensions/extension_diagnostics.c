// SPDX-License-Identifier: Apache-2.0

#include "Extensions/extension_diagnostics.h"

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Compiler/diagnostics.h"

#include <stdlib.h>

static SourceRange node_loc(const ASTNode* node) {
    if (node && node->location.start.line > 0) {
        return node->location;
    }
    SourceRange loc;
    loc.start.file = NULL;
    loc.start.line = node ? node->line : 0;
    loc.start.column = 0;
    loc.end = loc.start;
    return loc;
}

static void report_dim_mismatch(CompilerContext* ctx,
                                const ASTNode* node,
                                int code,
                                const char* hint,
                                const char* fmt,
                                FisicsDim8 lhsDim,
                                FisicsDim8 rhsDim) {
    char* lhsText = fisics_dim_to_string(lhsDim);
    char* rhsText = fisics_dim_to_string(rhsDim);
    compiler_report_diag(ctx,
                         node_loc(node),
                         DIAG_ERROR,
                         code,
                         hint,
                         fmt,
                         lhsText ? lhsText : "<oom>",
                         rhsText ? rhsText : "<oom>");
    free(lhsText);
    free(rhsText);
}

static void report_dim_binary_issue(CompilerContext* ctx,
                                    const ASTNode* node,
                                    int code,
                                    const char* hint,
                                    const char* fmt,
                                    const char* op,
                                    FisicsDim8 lhsDim,
                                    FisicsDim8 rhsDim) {
    char* lhsText = fisics_dim_to_string(lhsDim);
    char* rhsText = fisics_dim_to_string(rhsDim);
    compiler_report_diag(ctx,
                         node_loc(node),
                         DIAG_ERROR,
                         code,
                         hint,
                         fmt,
                         op ? op : "<op>",
                         lhsText ? lhsText : "<oom>",
                         rhsText ? rhsText : "<oom>");
    free(lhsText);
    free(rhsText);
}

void fisics_extension_diag_units_disabled(CompilerContext* ctx,
                                          const ASTNode* node,
                                          const char* exprText) {
    compiler_report_diag(ctx,
                         node_loc(node),
                         DIAG_ERROR,
                         CDIAG_EXTENSION_UNITS_DISABLED,
                         "enable with --overlay=physics-units or FISICS_OVERLAY=physics-units",
                         "fisics units annotation '%s' requires the physics-units overlay",
                         exprText ? exprText : "");
}

void fisics_extension_diag_invalid_units_expr(CompilerContext* ctx,
                                              const ASTNode* node,
                                              const char* exprText,
                                              const char* detail) {
    compiler_report_diag(ctx,
                         node_loc(node),
                         DIAG_ERROR,
                         CDIAG_EXTENSION_UNITS_INVALID_DIM,
                         detail,
                         "invalid fisics::dim expression '%s'",
                         exprText ? exprText : "");
}

void fisics_extension_diag_duplicate_units_attr(CompilerContext* ctx,
                                                const ASTNode* node) {
    compiler_report_diag(ctx,
                         node_loc(node),
                         DIAG_ERROR,
                         CDIAG_EXTENSION_UNITS_DUPLICATE,
                         "use only one [[fisics::dim(...)]] annotation per declaration node",
                         "duplicate fisics::dim annotation on declaration");
}

void fisics_extension_diag_units_assign_dim_mismatch(CompilerContext* ctx,
                                                     const ASTNode* node,
                                                     FisicsDim8 lhsDim,
                                                     FisicsDim8 rhsDim) {
    report_dim_mismatch(ctx,
                        node,
                        CDIAG_EXTENSION_UNITS_ASSIGN_DIM_MISMATCH,
                        "assignment requires identical dimensions on both sides",
                        "units assignment mismatch: lhs is '%s' but rhs is '%s'",
                        lhsDim,
                        rhsDim);
}

void fisics_extension_diag_units_add_dim_mismatch(CompilerContext* ctx,
                                                  const ASTNode* node,
                                                  FisicsDim8 lhsDim,
                                                  FisicsDim8 rhsDim) {
    report_dim_mismatch(ctx,
                        node,
                        CDIAG_EXTENSION_UNITS_ADD_DIM_MISMATCH,
                        "addition requires identical dimensions on both sides",
                        "units addition mismatch: lhs is '%s' but rhs is '%s'",
                        lhsDim,
                        rhsDim);
}

void fisics_extension_diag_units_sub_dim_mismatch(CompilerContext* ctx,
                                                  const ASTNode* node,
                                                  FisicsDim8 lhsDim,
                                                  FisicsDim8 rhsDim) {
    report_dim_mismatch(ctx,
                        node,
                        CDIAG_EXTENSION_UNITS_SUB_DIM_MISMATCH,
                        "subtraction requires identical dimensions on both sides",
                        "units subtraction mismatch: lhs is '%s' but rhs is '%s'",
                        lhsDim,
                        rhsDim);
}

void fisics_extension_diag_units_compare_dim_mismatch(CompilerContext* ctx,
                                                      const ASTNode* node,
                                                      FisicsDim8 lhsDim,
                                                      FisicsDim8 rhsDim) {
    report_dim_mismatch(ctx,
                        node,
                        CDIAG_EXTENSION_UNITS_COMPARE_DIM_MISMATCH,
                        "comparison requires identical dimensions on both sides",
                        "units comparison mismatch: lhs is '%s' but rhs is '%s'",
                        lhsDim,
                        rhsDim);
}

void fisics_extension_diag_units_exponent_overflow(CompilerContext* ctx,
                                                   const ASTNode* node,
                                                   const char* op,
                                                   FisicsDim8 lhsDim,
                                                   FisicsDim8 rhsDim) {
    report_dim_binary_issue(ctx,
                            node,
                            CDIAG_EXTENSION_UNITS_EXPONENT_OVERFLOW,
                            "dimension exponent math exceeded the int8 units range",
                            "units exponent overflow for '%s': lhs is '%s' and rhs is '%s'",
                            op,
                            lhsDim,
                            rhsDim);
}

void fisics_extension_diag_units_ternary_dim_mismatch(CompilerContext* ctx,
                                                      const ASTNode* node,
                                                      FisicsDim8 trueDim,
                                                      FisicsDim8 falseDim) {
    report_dim_binary_issue(ctx,
                            node,
                            CDIAG_EXTENSION_UNITS_UNSUPPORTED_OPERATION,
                            "ternary expressions require identical dimensions on both result branches",
                            "units ternary mismatch for '%s': true branch is '%s' but false branch is '%s'",
                            "?:",
                            trueDim,
                            falseDim);
}
