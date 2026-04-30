// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

#include "AST/ast_attribute.h"
#include "Extensions/Units/units_model.h"

bool fisics_units_dim_attribute_extract(const ASTAttribute* attr, char** outExprText);
bool fisics_units_unit_attribute_extract(const ASTAttribute* attr, char** outUnitText);
bool fisics_units_parse_dim_expr(const char* text, FisicsDim8* outDim, char** outErrorDetail);
