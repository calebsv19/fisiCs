// SPDX-License-Identifier: Apache-2.0

#include "Extensions/extension_diagnostics.h"

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Compiler/diagnostics.h"

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
