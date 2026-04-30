// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "Extensions/Units/units_model.h"

struct ASTNode;
struct CompilerContext;
struct Scope;

typedef struct FisicsUnitsAnnotation {
    /* Pipeline-lifetime owner key for the current declaration-side scaffold. */
    struct ASTNode* node;
    /* Compiler-owned copies of the source expression and canonical spelling. */
    char* exprText;
    char* canonicalText;
    FisicsDim8 dim;
    bool resolved;
    size_t duplicateCount;
} FisicsUnitsAnnotation;

typedef struct FisicsUnitsExprResult {
    /*
     * Reserved Phase 3 shape for inferred expression dimensions.
     * Keys are AST nodes and remain valid only for one compile pipeline run.
     */
    struct ASTNode* node;
    FisicsDim8 dim;
    bool resolved;
} FisicsUnitsExprResult;

typedef struct FisicsUnitsExprBinding {
    struct ASTNode* node;
    const FisicsUnitsAnnotation* annotation;
} FisicsUnitsExprBinding;

typedef struct FisicsExtensionState {
    /*
     * Current Phase 2/2.5 declaration-side storage.
     * One canonical slot per owner node, with duplicateCount recording repeated
     * [[fisics::dim(...)]] use against that same owner in the current scaffold.
     */
    FisicsUnitsAnnotation* unitsAnnotations;
    size_t unitsAnnotationCount;
    size_t unitsAnnotationCapacity;

    /*
     * Reserved expression-result side table for future units algebra.
     * This remains empty in Phase 2.5 and becomes populated in Phase 3+.
     */
    FisicsUnitsExprResult* unitsExprResults;
    size_t unitsExprResultCount;
    size_t unitsExprResultCapacity;

    /*
     * Identifier binding seam for Phase 5 expression inference.
     * Core semantic analysis records the resolved symbol for identifier nodes
     * while lexical scopes are still live so the extension expr pass can infer
     * declaration-owned dimensions without needing its own scope graph.
     */
    FisicsUnitsExprBinding* unitsExprBindings;
    size_t unitsExprBindingCount;
    size_t unitsExprBindingCapacity;
} FisicsExtensionState;

FisicsExtensionState* fisics_extension_state_create(void);
void fisics_extension_state_destroy(FisicsExtensionState* state);
bool fisics_extension_state_ensure(struct CompilerContext* ctx);

void fisics_extension_note_declaration(struct CompilerContext* ctx, struct ASTNode* node);
void fisics_extension_run_semantic_scaffold(struct ASTNode* root, struct Scope* globalScope);

size_t fisics_extension_units_annotation_count(const struct CompilerContext* ctx);
size_t fisics_extension_units_resolved_count(const struct CompilerContext* ctx);
