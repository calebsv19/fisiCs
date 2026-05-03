// SPDX-License-Identifier: Apache-2.0

#include "Extensions/extension_units_call_contracts.h"

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/Units/units_parser.h"
#include "Parser/Helpers/parsed_type.h"

#include <stdlib.h>
#include <string.h>

static bool units_call_arg_contract_reserve(FisicsExtensionState* state, size_t extra) {
    if (!state) return false;
    size_t need = state->unitsCallArgContractCount + extra;
    if (need <= state->unitsCallArgContractCapacity) return true;
    size_t newCap = state->unitsCallArgContractCapacity ? state->unitsCallArgContractCapacity * 2 : 8;
    while (newCap < need) newCap *= 2;
    FisicsUnitsCallArgContract* grown =
        (FisicsUnitsCallArgContract*)realloc(state->unitsCallArgContracts,
                                             newCap * sizeof(FisicsUnitsCallArgContract));
    if (!grown) return false;
    state->unitsCallArgContracts = grown;
    state->unitsCallArgContractCapacity = newCap;
    return true;
}

static FisicsUnitsCallArgContract* ensure_units_call_arg_contract(CompilerContext* ctx,
                                                                  const ASTNode* callNode,
                                                                  size_t argIndex) {
    if (!ctx || !callNode) return NULL;
    if (!fisics_extension_state_ensure(ctx)) return NULL;
    FisicsExtensionState* state = ctx->extensionState;
    for (size_t i = 0; i < state->unitsCallArgContractCount; ++i) {
        FisicsUnitsCallArgContract* contract = &state->unitsCallArgContracts[i];
        if (contract->callNode == callNode && contract->argIndex == argIndex) {
            return contract;
        }
    }
    if (!units_call_arg_contract_reserve(state, 1)) return NULL;
    FisicsUnitsCallArgContract* contract =
        &state->unitsCallArgContracts[state->unitsCallArgContractCount++];
    memset(contract, 0, sizeof(*contract));
    contract->callNode = (ASTNode*)callNode;
    contract->argIndex = argIndex;
    return contract;
}

static bool parse_param_units_contract(const ParsedType* paramType,
                                       FisicsDim8* outDim,
                                       bool* outResolved,
                                       const FisicsUnitDef** outUnitDef,
                                       bool* outUnitResolved) {
    if (outDim) *outDim = fisics_dim_zero();
    if (outResolved) *outResolved = false;
    if (outUnitDef) *outUnitDef = NULL;
    if (outUnitResolved) *outUnitResolved = false;
    if (!paramType || !paramType->attributes || paramType->attributeCount == 0) {
        return false;
    }

    bool sawAny = false;
    bool dimResolved = false;
    FisicsDim8 dim = fisics_dim_zero();
    const FisicsUnitDef* unitDef = NULL;
    bool unitResolved = false;

    for (size_t i = 0; i < paramType->attributeCount; ++i) {
        ASTAttribute* attr = paramType->attributes[i];
        char* dimText = NULL;
        if (!dimResolved && fisics_units_dim_attribute_extract(attr, &dimText)) {
            char* errorDetail = NULL;
            if (fisics_units_parse_dim_expr(dimText, &dim, &errorDetail)) {
                dimResolved = true;
                sawAny = true;
            }
            free(errorDetail);
            free(dimText);
            continue;
        }

        char* unitText = NULL;
        if (!unitResolved && fisics_units_unit_attribute_extract(attr, &unitText)) {
            if (unitText && fisics_unit_lookup(unitText, &unitDef) && unitDef) {
                unitResolved = true;
                sawAny = true;
            }
            free(unitText);
        }
    }

    if (!sawAny) return false;
    if (!dimResolved && unitResolved && unitDef) {
        dim = unitDef->dim;
        dimResolved = true;
    }
    if (dimResolved && unitResolved && unitDef && !fisics_dim_equal(dim, unitDef->dim)) {
        unitDef = NULL;
        unitResolved = false;
    }

    if (outDim) *outDim = dim;
    if (outResolved) *outResolved = dimResolved;
    if (outUnitDef) *outUnitDef = unitDef;
    if (outUnitResolved) *outUnitResolved = unitResolved && unitDef != NULL;
    return true;
}

bool fisics_extension_note_units_call_arg_contract_from_param_type(CompilerContext* ctx,
                                                                   const ASTNode* callNode,
                                                                   size_t argIndex,
                                                                   const ParsedType* paramType) {
    if (!ctx || !callNode || !paramType) return false;
    FisicsDim8 dim = fisics_dim_zero();
    bool resolved = false;
    const FisicsUnitDef* unitDef = NULL;
    bool unitResolved = false;
    if (!parse_param_units_contract(paramType, &dim, &resolved, &unitDef, &unitResolved)) {
        return false;
    }
    FisicsUnitsCallArgContract* contract = ensure_units_call_arg_contract(ctx, callNode, argIndex);
    if (!contract) return false;
    contract->dim = dim;
    contract->resolved = resolved;
    contract->unitDef = unitDef;
    contract->unitResolved = unitResolved;
    return true;
}

const FisicsUnitsCallArgContract* fisics_extension_lookup_units_call_arg_contract(
    const CompilerContext* ctx,
    const ASTNode* callNode,
    size_t argIndex) {
    if (!ctx || !ctx->extensionState || !callNode) return NULL;
    for (size_t i = 0; i < ctx->extensionState->unitsCallArgContractCount; ++i) {
        const FisicsUnitsCallArgContract* contract = &ctx->extensionState->unitsCallArgContracts[i];
        if (contract->callNode == callNode && contract->argIndex == argIndex) {
            return contract;
        }
    }
    return NULL;
}
