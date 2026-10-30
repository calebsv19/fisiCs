// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_frames.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#define PHYSICAL_MAX_DECLARED_CONTEXTS 65536u
/* Compiler-state contexts. Flags distinguish authored identity, checked static
 * compatibility and invariant scalars. None proves numeric geometry. */
#define PHYSICAL_FRAME_CHECKED 1u
#define PHYSICAL_FRAME_INVARIANT 2u
typedef struct PhysicalDeclaredContext {
  uint32_t frame, axis, applied_at, displacement_from, displacement_to;
  uint32_t moment_reference, point_subject, transform;
  uint32_t flags;
  uint32_t energy_form, energy_transfer; /* v7 independent qualifier handles */
} PhysicalDeclaredContext;
uint32_t physical_frame_intern(CompilerContext *, const ASTNode *,
                               PhysicalDeclaredContext, PhysicalFact *);
uint32_t physical_frame_bind(CompilerContext *, const ParsedType *,
                             const ASTNode *, PhysicalFact *, bool);
void physical_frame_validate_function(CompilerContext *, ASTNode *);
void physical_frame_components(CompilerContext *, const ParsedType *,
                               const ASTNode *, PhysicalFact *, bool);
const PhysicalDeclaredContext *
physical_declared_context(const FisicsPhysicalState *, uint32_t);

bool physical_frame_function_valid(CompilerContext *, ASTNode *);
