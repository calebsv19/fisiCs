// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stddef.h>
#include <stdio.h>

#include "Extensions/extension_hooks.h"

struct ASTNode;
struct CompilerContext;

typedef void (*FisicsUnitsAnnotationCallback)(const FisicsUnitsAnnotation* annotation,
                                              void* userData);

/*
 * Read-only traversal helpers for declaration-side units attachments.
 * These views are valid only for the lifetime of the current compile pipeline.
 */
const FisicsUnitsAnnotation* fisics_extension_lookup_units_annotation(
    const struct CompilerContext* ctx,
    const struct ASTNode* ownerNode);
const FisicsUnitsAnnotation* fisics_extension_units_annotation_at(
    const struct CompilerContext* ctx,
    size_t index);
void fisics_extension_for_each_units_annotation(const struct CompilerContext* ctx,
                                                FisicsUnitsAnnotationCallback callback,
                                                void* userData);

/*
 * Debug-only dump surface for semantic/AST inspection flows.
 * This does not define a stable external contract.
 */
void fisics_extension_dump_units_annotations(const struct CompilerContext* ctx, FILE* out);
