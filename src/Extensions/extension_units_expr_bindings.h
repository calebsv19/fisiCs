// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

struct ASTNode;
struct CompilerContext;
struct FisicsUnitsAnnotation;

bool fisics_extension_note_units_annotation_binding(
    struct CompilerContext* ctx,
    const struct ASTNode* node,
    const struct FisicsUnitsAnnotation* annotation);
const struct FisicsUnitsAnnotation* fisics_extension_lookup_units_annotation_binding(
    const struct CompilerContext* ctx,
    const struct ASTNode* node);
