// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

#include "Extensions/Units/units_model.h"
#include "Extensions/Units/units_registry.h"

struct ASTNode;
struct CompilerContext;

bool fisics_units_resolve_annotation(struct CompilerContext* ctx,
                                     const struct ASTNode* node,
                                     const char* exprText,
                                     bool overlayEnabled,
                                     FisicsDim8* outDim,
                                     char** outCanonical);
bool fisics_units_resolve_unit_annotation(struct CompilerContext* ctx,
                                          const struct ASTNode* node,
                                          const char* unitText,
                                          bool overlayEnabled,
                                          bool hasResolvedDim,
                                          FisicsDim8 declDim,
                                          const FisicsUnitDef** outUnitDef);
