// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
struct ASTNode;

/* Literal-only numeric values may take their meaning from an authored boundary. */
bool fisics_units_is_literal_value(const struct ASTNode* node);
