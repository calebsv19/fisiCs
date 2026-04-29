// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

#include "Extensions/Units/units_model.h"

struct ASTNode;
struct CompilerContext;

bool fisics_units_resolve_annotation(struct CompilerContext* ctx,
                                     const struct ASTNode* node,
                                     const char* exprText,
                                     bool overlayEnabled,
                                     FisicsDim8* outDim,
                                     char** outCanonical);
