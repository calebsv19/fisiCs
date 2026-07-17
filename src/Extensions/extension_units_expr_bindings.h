// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

struct ASTNode;
struct CompilerContext;
struct DesignatedInit;
struct FisicsUnitsAnnotation;
struct ParsedType;
struct Symbol;

bool fisics_extension_note_units_annotation_binding(
    struct CompilerContext* ctx,
    const struct ASTNode* node,
    const struct FisicsUnitsAnnotation* annotation);
bool fisics_extension_note_units_symbol_binding(
    struct CompilerContext* ctx,
    const struct ASTNode* node,
    const struct Symbol* symbol);
const struct FisicsUnitsAnnotation* fisics_extension_lookup_units_annotation_binding(
    const struct CompilerContext* ctx,
    const struct ASTNode* node);
const struct ParsedType* fisics_extension_lookup_units_symbol_type_binding(
    const struct CompilerContext* ctx,
    const struct ASTNode* node);
struct DesignatedInit* fisics_extension_lookup_units_symbol_initializer_binding(
    const struct CompilerContext* ctx,
    const struct ASTNode* node);
