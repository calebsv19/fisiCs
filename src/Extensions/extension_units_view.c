// SPDX-License-Identifier: Apache-2.0

#include "Extensions/extension_units_view.h"

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"

#include <stdio.h>

static FILE* effective_out(FILE* out) {
    return out ? out : stdout;
}

const FisicsUnitsAnnotation* fisics_extension_lookup_units_annotation(
    const CompilerContext* ctx,
    const ASTNode* ownerNode) {
    if (!ctx || !ctx->extensionState || !ownerNode) return NULL;
    for (size_t i = 0; i < ctx->extensionState->unitsAnnotationCount; ++i) {
        const FisicsUnitsAnnotation* ann = &ctx->extensionState->unitsAnnotations[i];
        if (ann->node == ownerNode) {
            return ann;
        }
    }
    return NULL;
}

const FisicsUnitsAnnotation* fisics_extension_units_annotation_at(
    const CompilerContext* ctx,
    size_t index) {
    if (!ctx || !ctx->extensionState || index >= ctx->extensionState->unitsAnnotationCount) {
        return NULL;
    }
    return &ctx->extensionState->unitsAnnotations[index];
}

void fisics_extension_for_each_units_annotation(const CompilerContext* ctx,
                                                FisicsUnitsAnnotationCallback callback,
                                                void* userData) {
    if (!ctx || !ctx->extensionState || !callback) return;
    for (size_t i = 0; i < ctx->extensionState->unitsAnnotationCount; ++i) {
        callback(&ctx->extensionState->unitsAnnotations[i], userData);
    }
}

void fisics_extension_dump_units_annotations(const CompilerContext* ctx, FILE* out) {
    FILE* stream = effective_out(out);
    size_t count = fisics_extension_units_annotation_count(ctx);
    if (count == 0) {
        fprintf(stream, "Extension Units Annotations: 0\n");
        return;
    }

    fprintf(stream, "Extension Units Annotations: %zu\n", count);
    for (size_t i = 0; i < count; ++i) {
        const FisicsUnitsAnnotation* ann = fisics_extension_units_annotation_at(ctx, i);
        if (!ann) continue;
        unsigned line = 0;
        if (ann->node) {
            line = ann->node->location.start.line ? ann->node->location.start.line : (unsigned)ann->node->line;
        }
        const char* text = ann->canonicalText ? ann->canonicalText
                                              : (ann->exprText ? ann->exprText : "<missing>");
        fprintf(stream,
                "  - line %u: %s [%s",
                line,
                text,
                ann->resolved ? "resolved" : "pending");
        if (ann->duplicateCount > 1) {
            fprintf(stream, ", duplicates=%zu", ann->duplicateCount);
        }
        fprintf(stream, "]\n");
    }
}
