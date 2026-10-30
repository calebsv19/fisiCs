// SPDX-License-Identifier: Apache-2.0

#include "Extensions/extension_units_expr_bindings.h"

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_hooks.h"
#include "Syntax/symbol_table.h"

#include <stdlib.h>
#include <string.h>

static void units_expr_binding_replace_symbol_snapshot(FisicsUnitsExprBinding* binding,
                                                       const Symbol* symbol) {
    if (!binding || !symbol) return;
    if (binding->hasSymbolType) {
        parsedTypeFree(&binding->symbolType);
        memset(&binding->symbolType, 0, sizeof(binding->symbolType));
        binding->hasSymbolType = false;
    }
    binding->symbolType = parsedTypeClone(&symbol->type);
    binding->hasSymbolType = true;
    binding->originNode = symbol->definition;
    if (symbol->definition && symbol->definition->type == AST_VARIABLE_DECLARATION) {
        ASTNode* decl = symbol->definition;
        for (size_t i = 0; i < decl->varDecl.varCount; ++i) {
            ASTNode* ident = decl->varDecl.varNames ? decl->varDecl.varNames[i] : NULL;
            if (ident && ident->valueNode.value && symbol->name &&
                strcmp(ident->valueNode.value, symbol->name) == 0) {
                binding->originNode = ident;
                break;
            }
        }
    }
    binding->initializer = symbol->initializer;
    binding->functionDefinition = symbol->kind == SYMBOL_FUNCTION ? symbol->definition : NULL;
}

static bool units_expr_binding_reserve(FisicsExtensionState* state, size_t extra) {
    if (!state) return false;
    size_t need = state->unitsExprBindingCount + extra;
    if (need <= state->unitsExprBindingCapacity) return true;
    size_t newCap = state->unitsExprBindingCapacity ? state->unitsExprBindingCapacity * 2 : 8;
    while (newCap < need) newCap *= 2;
    FisicsUnitsExprBinding* grown =
        (FisicsUnitsExprBinding*)realloc(state->unitsExprBindings,
                                         newCap * sizeof(FisicsUnitsExprBinding));
    if (!grown) return false;
    state->unitsExprBindings = grown;
    state->unitsExprBindingCapacity = newCap;
    return true;
}

bool fisics_extension_note_units_annotation_binding(CompilerContext* ctx,
                                                    const ASTNode* node,
                                                    const FisicsUnitsAnnotation* annotation) {
    if (!ctx || !node || !annotation) return false;
    if (!fisics_extension_state_ensure(ctx)) return false;
    FisicsExtensionState* state = ctx->extensionState;
    for (size_t i = 0; i < state->unitsExprBindingCount; ++i) {
        FisicsUnitsExprBinding* binding = &state->unitsExprBindings[i];
        if (binding->node == node) {
            binding->annotation = annotation;
            return true;
        }
    }
    if (!units_expr_binding_reserve(state, 1)) return false;
    FisicsUnitsExprBinding* binding = &state->unitsExprBindings[state->unitsExprBindingCount++];
    memset(binding, 0, sizeof(*binding));
    binding->node = (ASTNode*)node;
    binding->annotation = annotation;
    return true;
}

bool fisics_extension_note_units_symbol_binding(CompilerContext* ctx,
                                                const ASTNode* node,
                                                const Symbol* symbol) {
    if (!ctx || !node || !symbol) return false;
    FisicsExtensionState* state = ctx->extensionState;
    if (!state) return false;
    const FisicsUnitsAnnotation* annotation = symbolGetUnitsAnnotation(symbol);
    for (size_t i = 0; i < state->unitsExprBindingCount; ++i) {
        FisicsUnitsExprBinding* binding = &state->unitsExprBindings[i];
        if (binding->node == node) {
            binding->annotation = annotation;
            units_expr_binding_replace_symbol_snapshot(binding, symbol);
            return true;
        }
    }
    if (!units_expr_binding_reserve(state, 1)) return false;
    FisicsUnitsExprBinding* binding = &state->unitsExprBindings[state->unitsExprBindingCount++];
    memset(binding, 0, sizeof(*binding));
    binding->node = (ASTNode*)node;
    binding->annotation = annotation;
    units_expr_binding_replace_symbol_snapshot(binding, symbol);
    return true;
}

const FisicsUnitsAnnotation* fisics_extension_lookup_units_annotation_binding(
    const CompilerContext* ctx,
    const ASTNode* node) {
    if (!ctx || !ctx->extensionState || !node) return NULL;
    for (size_t i = 0; i < ctx->extensionState->unitsExprBindingCount; ++i) {
        const FisicsUnitsExprBinding* binding = &ctx->extensionState->unitsExprBindings[i];
        if (binding->node == node) {
            return binding->annotation;
        }
    }
    return NULL;
}

const ParsedType* fisics_extension_lookup_units_symbol_type_binding(
    const CompilerContext* ctx,
    const ASTNode* node) {
    if (!ctx || !ctx->extensionState || !node) return NULL;
    for (size_t i = 0; i < ctx->extensionState->unitsExprBindingCount; ++i) {
        const FisicsUnitsExprBinding* binding = &ctx->extensionState->unitsExprBindings[i];
        if (binding->node == node) {
            return binding->hasSymbolType ? &binding->symbolType : NULL;
        }
    }
    return NULL;
}

DesignatedInit* fisics_extension_lookup_units_symbol_initializer_binding(
    const CompilerContext* ctx,
    const ASTNode* node) {
    if (!ctx || !ctx->extensionState || !node) return NULL;
    for (size_t i = 0; i < ctx->extensionState->unitsExprBindingCount; ++i) {
        const FisicsUnitsExprBinding* binding = &ctx->extensionState->unitsExprBindings[i];
        if (binding->node == node) {
            return binding->initializer;
        }
    }
    return NULL;
}

ASTNode* fisics_extension_lookup_units_function_binding(const CompilerContext* ctx,const ASTNode* node) {
    if (!ctx || !ctx->extensionState || !node) return NULL;
    for (size_t i=0;i<ctx->extensionState->unitsExprBindingCount;++i) {
        const FisicsUnitsExprBinding* binding=&ctx->extensionState->unitsExprBindings[i];
        if (binding->node==node) return binding->functionDefinition;
    }
    return NULL;
}

bool fisics_extension_note_units_type_binding(CompilerContext* ctx,
                                               const ASTNode* node,
                                               const ParsedType* type) {
    if (!ctx || !node || !type || !fisics_extension_state_ensure(ctx)) return false;
    FisicsExtensionState* state = ctx->extensionState;
    FisicsUnitsExprBinding* binding = NULL;
    for (size_t i = 0; i < state->unitsExprBindingCount; ++i) {
        if (state->unitsExprBindings[i].node == node) {
            binding = &state->unitsExprBindings[i];
            break;
        }
    }
    if (!binding) {
        if (!units_expr_binding_reserve(state, 1)) return false;
        binding = &state->unitsExprBindings[state->unitsExprBindingCount++];
        memset(binding, 0, sizeof(*binding));
        binding->node = (ASTNode*)node;
    }
    if (binding->hasSymbolType) parsedTypeFree(&binding->symbolType);
    binding->symbolType = parsedTypeClone(type);
    binding->hasSymbolType = binding->symbolType.kind != TYPE_INVALID;
    return binding->hasSymbolType;
}

const ASTNode* fisics_units_lookup_origin(const CompilerContext* ctx, const ASTNode* node) {
    if (!ctx || !ctx->extensionState) return NULL;
    for (size_t i=0;i<ctx->extensionState->unitsExprBindingCount;++i) {
        const FisicsUnitsExprBinding* binding=&ctx->extensionState->unitsExprBindings[i];
        if (binding->node==node) return binding->originNode;
    }
    return NULL;
}
void fisics_units_note_origin(CompilerContext* ctx,const ASTNode* node,const ASTNode* origin) {
    if (!ctx || !ctx->extensionState || !origin) return;
    for (size_t i=0;i<ctx->extensionState->unitsExprBindingCount;++i)
        if (ctx->extensionState->unitsExprBindings[i].node==node) {
            ctx->extensionState->unitsExprBindings[i].originNode=(ASTNode*)origin;
            return;
        }
}
