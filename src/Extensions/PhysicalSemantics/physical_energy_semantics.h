// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_energy.h"
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
void physical_energy_bind(CompilerContext *, const ParsedType *,
                          const ASTNode *, PhysicalFact *, bool);
PhysicalFact physical_energy_boundary(CompilerContext *, const ASTNode *,
                                      PhysicalFact, PhysicalFact, PhysicalFact,
                                      bool);
PhysicalFact physical_energy_binary(CompilerContext *, const ASTNode *,
                                    PhysicalFact, PhysicalFact, PhysicalFact);
PhysicalFact physical_energy_scale(CompilerContext *, const ASTNode *,
                                   const char *, PhysicalFact, PhysicalFact,
                                   PhysicalFact);
void physical_energy_validate_function(CompilerContext *, ASTNode *);
bool physical_energy_transfer_call(CompilerContext *, ASTNode *,
                                   PhysicalFact *);
PhysicalFact physical_energy_call_result(CompilerContext *, ASTNode *,
                                         PhysicalFact);
