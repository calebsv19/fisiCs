// SPDX-License-Identifier: Apache-2.0

#include "Extensions/extension_hooks.h"

#include <stdlib.h>
#include <string.h>

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_diagnostics.h"
#include "Extensions/Units/units_decl_semantics.h"
#include "Extensions/Units/units_expr_semantics.h"
#include "Extensions/Units/units_parser.h"
#include "Syntax/scope.h"

FisicsExtensionState* fisics_extension_state_create(void) {
    return (FisicsExtensionState*)calloc(1, sizeof(FisicsExtensionState));
}

void fisics_extension_state_destroy(FisicsExtensionState* state) {
    if (!state) return;
    for (size_t i = 0; i < state->unitsAnnotationCount; ++i) {
        free(state->unitsAnnotations[i].exprText);
        free(state->unitsAnnotations[i].canonicalText);
    }
    free(state->unitsAnnotations);
    free(state);
}

bool fisics_extension_state_ensure(CompilerContext* ctx) {
    if (!ctx) return false;
    if (ctx->extensionState) return true;
    ctx->extensionState = fisics_extension_state_create();
    return ctx->extensionState != NULL;
}

static bool units_annotation_reserve(FisicsExtensionState* state, size_t extra) {
    if (!state) return false;
    size_t need = state->unitsAnnotationCount + extra;
    if (need <= state->unitsAnnotationCapacity) return true;
    size_t newCap = state->unitsAnnotationCapacity ? state->unitsAnnotationCapacity * 2 : 4;
    while (newCap < need) newCap *= 2;
    FisicsUnitsAnnotation* grown = (FisicsUnitsAnnotation*)realloc(
        state->unitsAnnotations,
        newCap * sizeof(FisicsUnitsAnnotation));
    if (!grown) return false;
    state->unitsAnnotations = grown;
    state->unitsAnnotationCapacity = newCap;
    return true;
}

static bool note_units_attr(CompilerContext* ctx, ASTNode* node, char* exprText) {
    if (!ctx || !node || !exprText) {
        free(exprText);
        return false;
    }
    if (!fisics_extension_state_ensure(ctx)) {
        free(exprText);
        return false;
    }
    FisicsExtensionState* state = ctx->extensionState;
    if (!units_annotation_reserve(state, 1)) {
        free(exprText);
        return false;
    }
    FisicsUnitsAnnotation* ann = &state->unitsAnnotations[state->unitsAnnotationCount++];
    memset(ann, 0, sizeof(*ann));
    ann->node = node;
    ann->exprText = exprText;
    return true;
}

static size_t note_units_attrs_from_list(CompilerContext* ctx,
                                         ASTNode* node,
                                         ASTAttribute** attrs,
                                         size_t attrCount) {
    if (!ctx || !node || !attrs || attrCount == 0) return 0;
    size_t noted = 0;
    for (size_t i = 0; i < attrCount; ++i) {
        char* exprText = NULL;
        if (!fisics_units_attribute_extract(attrs[i], &exprText)) {
            continue;
        }
        (void)note_units_attr(ctx, node, exprText);
        noted++;
    }
    return noted;
}

void fisics_extension_note_declaration(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node) return;
    size_t noted = note_units_attrs_from_list(ctx, node, node->attributes, node->attributeCount);
    if (noted == 0 && node->type == AST_VARIABLE_DECLARATION) {
        const ParsedType* firstType = astVarDeclTypeAt(node, 0);
        if (firstType) {
            (void)note_units_attrs_from_list(ctx,
                                             node,
                                             firstType->attributes,
                                             firstType->attributeCount);
        }
    }
}

static void run_units_decl_scaffold(CompilerContext* ctx) {
    if (!ctx || !ctx->extensionState) return;
    FisicsExtensionState* state = ctx->extensionState;
    bool overlayEnabled = cc_overlay_physics_units_enabled(ctx);
    for (size_t i = 0; i < state->unitsAnnotationCount; ++i) {
        FisicsUnitsAnnotation* ann = &state->unitsAnnotations[i];
        if (!ann->node || !ann->exprText) continue;

        size_t duplicates = 0;
        for (size_t j = 0; j < state->unitsAnnotationCount; ++j) {
            if (state->unitsAnnotations[j].node == ann->node) {
                duplicates++;
            }
        }
        if (duplicates > 1) {
            fisics_extension_diag_duplicate_units_attr(ctx, ann->node);
            continue;
        }

        free(ann->canonicalText);
        ann->canonicalText = NULL;
        ann->resolved = fisics_units_resolve_annotation(ctx,
                                                        ann->node,
                                                        ann->exprText,
                                                        overlayEnabled,
                                                        &ann->dim,
                                                        &ann->canonicalText);
    }
}

void fisics_extension_run_semantic_scaffold(ASTNode* root, Scope* globalScope) {
    if (!globalScope || !globalScope->ctx) return;
    CompilerContext* ctx = globalScope->ctx;
    run_units_decl_scaffold(ctx);
    fisics_units_run_expr_semantics(root, globalScope);
}

size_t fisics_extension_units_annotation_count(const CompilerContext* ctx) {
    return (ctx && ctx->extensionState) ? ctx->extensionState->unitsAnnotationCount : 0;
}

size_t fisics_extension_units_resolved_count(const CompilerContext* ctx) {
    if (!ctx || !ctx->extensionState) return 0;
    size_t count = 0;
    for (size_t i = 0; i < ctx->extensionState->unitsAnnotationCount; ++i) {
        if (ctx->extensionState->unitsAnnotations[i].resolved) {
            count++;
        }
    }
    return count;
}
