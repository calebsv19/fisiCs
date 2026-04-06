// SPDX-License-Identifier: Apache-2.0

#include "code_gen.h"

#include "codegen_private.h"

#include <stdio.h>

LLVMValueRef codegen_generate(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node) {
        return NULL;
    }

    CG_DEBUG("[CG] Generating code for root node type=%d\n", node->type);
    if (!cg_scope_push(ctx)) {
        fprintf(stderr, "Error: failed to allocate initial codegen scope\n");
        return NULL;
    }

    if (node->type == AST_PROGRAM) {
        predeclareGlobals(ctx, node);
    }

    LLVMValueRef result = codegenNode(ctx, node);
    cg_scope_pop(ctx);
    return result;
}
