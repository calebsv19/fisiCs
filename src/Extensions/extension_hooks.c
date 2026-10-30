// SPDX-License-Identifier: Apache-2.0

#include "Extensions/extension_hooks.h"
#include "Extensions/PhysicalSemantics/physical_semantics.h"

#include <stdlib.h>
#include <string.h>

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_diagnostics.h"
#include "Extensions/SemanticMetadata/semantic_metadata_semantics.h"
#include "Extensions/Units/units_decl_semantics.h"
#include "Extensions/Units/units_expr_semantics.h"
#include "Extensions/Units/units_parser.h"
#include "Syntax/scope.h"

FisicsExtensionState* fisics_extension_state_create(void) {
    return (FisicsExtensionState*)calloc(1, sizeof(FisicsExtensionState));
}

void fisics_extension_state_destroy(FisicsExtensionState* state) {
    if (!state) return;
    fisics_physical_state_free(state->physicalState);
    fisics_semantic_metadata_records_destroy(state->semanticMetadataRecords,
                                             state->semanticMetadataRecordCount);
    fisics_semantic_metadata_edges_destroy(state->semanticMetadataEdges,
                                           state->semanticMetadataEdgeCount);
    for (size_t i = 0; i < state->unitsAnnotationCount; ++i) {
        free(state->unitsAnnotations[i].dimExprText);
        free(state->unitsAnnotations[i].canonicalText);
        free(state->unitsAnnotations[i].unitExprText);
    }
    for (size_t i = 0; i < state->unitsExprBindingCount; ++i) {
        if (state->unitsExprBindings[i].hasSymbolType) {
            parsedTypeFree(&state->unitsExprBindings[i].symbolType);
        }
    }
    for (size_t i=0;i<state->unitsSourceFileCount;++i)free(state->unitsSourceFiles[i]);
    free(state->unitsSourceFiles);
    free(state->unitsAnnotations);
    free(state->unitsExprResults);
    free(state->unitsExprBindings);
    free(state->unitsCallArgContracts);
    free(state->unitsMathCalls);
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

static FisicsUnitsAnnotation* find_units_annotation(FisicsExtensionState* state, ASTNode* node, size_t declaratorIndex) {
    if (!state || !node) return NULL;
    for (size_t i = 0; i < state->unitsAnnotationCount; ++i) {
        if (state->unitsAnnotations[i].node == node &&
            state->unitsAnnotations[i].declaratorIndex == declaratorIndex) {
            return &state->unitsAnnotations[i];
        }
    }
    return NULL;
}

static FisicsUnitsAnnotation* ensure_units_annotation(CompilerContext* ctx, ASTNode* node, size_t declaratorIndex) {
    if (!ctx || !node) return NULL;
    if (!fisics_extension_state_ensure(ctx)) {
        return NULL;
    }
    FisicsExtensionState* state = ctx->extensionState;
    FisicsUnitsAnnotation* existing = find_units_annotation(state, node, declaratorIndex);
    if (existing) {
        return existing;
    }
    if (!units_annotation_reserve(state, 1)) {
        return NULL;
    }
    FisicsUnitsAnnotation* ann = &state->unitsAnnotations[state->unitsAnnotationCount++];
    memset(ann, 0, sizeof(*ann));
    ann->node = node;
    ann->declaratorIndex = declaratorIndex;
    return ann;
}

static bool note_units_dim_attr(CompilerContext* ctx, ASTNode* node, size_t declaratorIndex, char* exprText) {
    if (!ctx || !node || !exprText) {
        free(exprText);
        return false;
    }
    FisicsUnitsAnnotation* ann = ensure_units_annotation(ctx, node, declaratorIndex);
    if (!ann) {
        free(exprText);
        return false;
    }
    if (ann->dimExprText) {
        ann->dimDuplicateCount++;
        free(exprText);
        return true;
    }
    ann->dimExprText = exprText;
    ann->dimDuplicateCount = 1;
    return true;
}

static bool note_units_unit_attr(CompilerContext* ctx, ASTNode* node, size_t declaratorIndex, char* unitText) {
    if (!ctx || !node || !unitText) {
        free(unitText);
        return false;
    }
    FisicsUnitsAnnotation* ann = ensure_units_annotation(ctx, node, declaratorIndex);
    if (!ann) {
        free(unitText);
        return false;
    }
    if (ann->unitExprText) {
        ann->unitDuplicateCount++;
        free(unitText);
        return true;
    }
    ann->unitExprText = unitText;
    ann->unitDuplicateCount = 1;
    return true;
}

static size_t note_units_attrs_from_list(CompilerContext* ctx,
                                         ASTNode* node,
                                         size_t declaratorIndex,
                                         ASTAttribute** attrs,
                                         size_t attrCount) {
    if (!ctx || !node || !attrs || attrCount == 0) return 0;
    size_t noted = 0;
    for (size_t i = 0; i < attrCount; ++i) {
        char* exprText = NULL;
        if (fisics_units_dim_attribute_extract(attrs[i], &exprText)) {
            (void)note_units_dim_attr(ctx, node, declaratorIndex, exprText);
            noted++;
            continue;
        }
        char* unitText = NULL;
        if (fisics_units_unit_attribute_extract(attrs[i], &unitText)) {
            (void)note_units_unit_attr(ctx, node, declaratorIndex, unitText);
            noted++;
        }
    }
    return noted;
}

void fisics_extension_note_units_prefix(CompilerContext* ctx, ASTNode* node, ASTAttribute** attrs, size_t count) {
    size_t slots = node->type == AST_VARIABLE_DECLARATION ? node->varDecl.varCount : 1;
    for (size_t i=0;i<slots;i++) (void)note_units_attrs_from_list(ctx,node,i,attrs,count);
}

void fisics_extension_note_declaration(CompilerContext* ctx, ASTNode* node) {
    if (!ctx || !node) return;
    if (node->type == AST_VARIABLE_DECLARATION) {
        /* Each ParsedType already contains its shared specifier attributes and
         * its own declarator attributes. Never reuse the first variable's slot. */
        for (size_t i = 0; i < node->varDecl.varCount; ++i) {
            const ParsedType* type = astVarDeclTypeAt(node, i);
            if (type && type->attributeCount) {
                (void)note_units_attrs_from_list(ctx, node, i, type->attributes, type->attributeCount);
            } else if (!node->varDecl.declaredTypes) {
                (void)note_units_attrs_from_list(ctx, node, i, node->attributes, node->attributeCount);
            }
        }
        return;
    }
    size_t noted = note_units_attrs_from_list(ctx, node, 0, node->attributes, node->attributeCount);
    if (noted == 0 && (node->type == AST_FUNCTION_DECLARATION || node->type == AST_FUNCTION_DEFINITION)) {
        const ParsedType* type = node->type == AST_FUNCTION_DEFINITION
            ? &node->functionDef.returnType : &node->functionDecl.returnType;
        (void)note_units_attrs_from_list(ctx, node, 0, type->attributes, type->attributeCount);
    }
}

static void run_units_decl_scaffold(CompilerContext* ctx) {
    if (!ctx || !ctx->extensionState) return;
    FisicsExtensionState* state = ctx->extensionState;
    bool overlayEnabled = cc_overlay_physics_units_enabled(ctx);
    for (size_t i = 0; i < state->unitsAnnotationCount; ++i) {
        FisicsUnitsAnnotation* ann = &state->unitsAnnotations[i];
        if (!ann->node) continue;
        ASTNode* diagnosticOwner = ann->node;
        if (ann->node->type == AST_VARIABLE_DECLARATION && ann->node->varDecl.varCount > 1 &&
            ann->node->varDecl.varNames && ann->declaratorIndex < ann->node->varDecl.varCount &&
            ann->node->varDecl.varNames[ann->declaratorIndex]) {
            diagnosticOwner = ann->node->varDecl.varNames[ann->declaratorIndex];
        }

        if (ann->dimDuplicateCount > 1) {
            fisics_extension_diag_duplicate_units_attr(ctx, diagnosticOwner);
        }
        if (ann->unitDuplicateCount > 1) {
            fisics_extension_diag_duplicate_unit_attr(ctx, diagnosticOwner);
        }

        free(ann->canonicalText);
        ann->canonicalText = NULL;
        ann->resolved = false;
        ann->unitDef = NULL;
        ann->unitResolved = false;

        if (ann->dimExprText && ann->dimDuplicateCount <= 1) {
            ann->resolved = fisics_units_resolve_annotation(ctx,
                                                            diagnosticOwner,
                                                            ann->dimExprText,
                                                            overlayEnabled,
                                                            &ann->dim,
                                                            &ann->canonicalText);
        }

        if (ann->unitExprText && ann->unitDuplicateCount <= 1) {
            ann->unitResolved = fisics_units_resolve_unit_annotation(ctx,
                                                                     diagnosticOwner,
                                                                     ann->unitExprText,
                                                                     overlayEnabled,
                                                                     ann->resolved,
                                                                     ann->dim,
                                                                     &ann->unitDef);
        }
    }
}

void fisics_extension_run_semantic_scaffold(ASTNode* root, Scope* globalScope) {
    if (!globalScope || !globalScope->ctx) return;
    CompilerContext* ctx = globalScope->ctx;
    run_units_decl_scaffold(ctx);
    fisics_units_run_expr_semantics(root, globalScope);
    fisics_physical_run(root, ctx);
    fisics_semantic_metadata_run(root, globalScope);
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
