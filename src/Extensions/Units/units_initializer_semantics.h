// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stddef.h>
#include <stdbool.h>

struct ASTNode;
struct CompilerContext;

/* Validate a scalar expression or annotated array's initializer entries against
 * the declaration/field that owns their physical dimension and optional unit.
 * declarationOwned preserves direct number/char declaration metadata adoption;
 * record-field and nested array constants keep their existing side-table results. */
void fisics_units_validate_initializer(struct CompilerContext* ctx,
                                       struct ASTNode* owner,
                                       struct ASTNode* expression,
                                       bool declarationOwned);

void fisics_units_validate_declarator_initializer(struct CompilerContext* ctx,
    struct ASTNode* owner, size_t declaratorIndex, struct ASTNode* expression);
