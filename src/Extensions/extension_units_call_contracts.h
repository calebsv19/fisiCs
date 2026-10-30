// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

struct ASTNode;
struct CompilerContext;
struct ParsedType;

#include "Extensions/extension_hooks.h"

bool fisics_extension_note_units_call_arg_contract_from_param_type(
    struct CompilerContext* ctx,
    const struct ASTNode* callNode,
    size_t argIndex,
    const struct ParsedType* paramType);

const FisicsUnitsCallArgContract* fisics_extension_lookup_units_call_arg_contract(
    const struct CompilerContext* ctx,
    const struct ASTNode* callNode,
    size_t argIndex);

/* Return contracts use a reserved key, never an argument index. This also
 * stores the effective definition contract while lexical scopes are live. */
#define FISICS_UNITS_RETURN_CONTRACT_INDEX ((size_t)-1)
bool fisics_units_parse_type_contract(const struct ParsedType* type,
    FisicsDim8* dim, bool* resolved, const FisicsUnitDef** unit, bool* unitResolved);
bool fisics_units_return_contracts_compatible(const struct ParsedType* a,
                                              const struct ParsedType* b);
void fisics_units_note_function_return_contract(struct CompilerContext* ctx,
    struct ASTNode* node, const struct ParsedType* previous, struct ParsedType* current);

void fisics_units_inherit_return_attributes(struct ParsedType* current,const struct ParsedType* previous);
