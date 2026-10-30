// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_frame_binding.h"
PhysicalDeclaredContext physical_frame_context(CompilerContext *, PhysicalFact);
bool physical_frame_geometric(CompilerContext *, PhysicalFact);
PhysicalFact physical_frame_boundary(CompilerContext *, const ASTNode *,
                                     const char *, PhysicalFact, PhysicalFact,
                                     PhysicalFact, bool);
PhysicalFact physical_frame_binary(CompilerContext *, const ASTNode *,
                                   const char *, PhysicalFact, PhysicalFact,
                                   PhysicalFact);
PhysicalFact physical_frame_scale(CompilerContext *, const ASTNode *,
                                  const char *, PhysicalFact, PhysicalFact,
                                  PhysicalFact);
PhysicalFact physical_frame_project(CompilerContext *, const ASTNode *,
                                    PhysicalFact, unsigned);
PhysicalFact physical_frame_sign(CompilerContext *, const ASTNode *,
                                 PhysicalFact);
PhysicalFact physical_frame_shape(CompilerContext *, const ASTNode *, unsigned,
                                  PhysicalFact, PhysicalFact, PhysicalFact);

PhysicalFact physical_frame_call_result(CompilerContext *, const ASTNode *,
                                        PhysicalFact, PhysicalFact);

bool physical_frame_transform_call(CompilerContext *, ASTNode *, PhysicalFact *);

bool physical_point_operation(CompilerContext *, const ASTNode *, const char *, PhysicalFact, PhysicalFact, PhysicalFact *);
