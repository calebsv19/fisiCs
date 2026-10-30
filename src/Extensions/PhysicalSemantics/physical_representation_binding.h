// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_representations.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
bool physical_representation_bind(CompilerContext *, const ParsedType *,
                                  const ASTNode *, uint32_t, uint32_t, uint32_t, bool);
bool physical_representation_access(CompilerContext *, ASTNode *,
                                    PhysicalFact *);
bool physical_representation_initializer(CompilerContext *, const ParsedType *,
                                         ASTNode *, ASTNode *);
bool physical_shape_call(CompilerContext *, ASTNode *, PhysicalFact *);
void physical_shape_validate_function(CompilerContext *, ASTNode *);
char *physical_shape_attribute(const ParsedType *, const char *name);
