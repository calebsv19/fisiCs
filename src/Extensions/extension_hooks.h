// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "Extensions/Units/units_model.h"

struct ASTNode;
struct CompilerContext;
struct Scope;

typedef struct FisicsUnitsAnnotation {
    struct ASTNode* node;
    char* exprText;
    char* canonicalText;
    FisicsDim8 dim;
    bool resolved;
} FisicsUnitsAnnotation;

typedef struct FisicsExtensionState {
    FisicsUnitsAnnotation* unitsAnnotations;
    size_t unitsAnnotationCount;
    size_t unitsAnnotationCapacity;
} FisicsExtensionState;

FisicsExtensionState* fisics_extension_state_create(void);
void fisics_extension_state_destroy(FisicsExtensionState* state);
bool fisics_extension_state_ensure(struct CompilerContext* ctx);

void fisics_extension_note_declaration(struct CompilerContext* ctx, struct ASTNode* node);
void fisics_extension_run_semantic_scaffold(struct ASTNode* root, struct Scope* globalScope);

size_t fisics_extension_units_annotation_count(const struct CompilerContext* ctx);
size_t fisics_extension_units_resolved_count(const struct CompilerContext* ctx);
