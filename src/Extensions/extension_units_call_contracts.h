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
