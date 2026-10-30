// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
struct CompilerContext;
#include "Lexer/token_buffer.h"
bool fisics_units_retain_source_ranges(struct CompilerContext* ctx,TokenBuffer* tokens);
