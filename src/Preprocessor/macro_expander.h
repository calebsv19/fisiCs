// SPDX-License-Identifier: Apache-2.0

#ifndef PREPROCESSOR_MACRO_EXPANDER_H
#define PREPROCESSOR_MACRO_EXPANDER_H

#include <stdbool.h>
#include <stddef.h>

#include "Preprocessor/macro_table.h"

typedef struct {
    Token* tokens;
    size_t count;
    size_t capacity;
} PPTokenBuffer;

typedef struct {
    const char* baseFile;
    const char* dateString;
    const char* timeString;
    int* counter;
    const char** logicalFile;
    int* lineOffset;
    bool* lineRemapActive;
} PPBuiltinState;

typedef enum {
    ME_ERR_NONE = 0,
    ME_ERR_MACRO_ARG_COUNT = 1,
    ME_ERR_UNSUPPORTED_GNU_COMMA_VA_ARGS = 2
} MacroExpandError;

typedef struct {
    MacroExpandError kind;
    const MacroDefinition* macro;
    SourceRange callSite;
    size_t expectedArgs;
    size_t providedArgs;
    bool variadic;
} MacroExpandErrorInfo;

typedef struct {
    MacroTable* table;
    PPBuiltinState builtins;
    MacroExpandErrorInfo lastError;
    bool preserveDefinedOperands;
} MacroExpander;

void macro_expander_init(MacroExpander* expander, MacroTable* table);
void macro_expander_reset(MacroExpander* expander);
void macro_expander_set_builtins(MacroExpander* expander, PPBuiltinState state);
void macro_expander_set_preserve_defined_operands(MacroExpander* expander, bool enabled);

bool macro_expander_expand(MacroExpander* expander,
                           const Token* input,
                           size_t count,
                           PPTokenBuffer* outTokens);

MacroExpandErrorInfo macro_expander_last_error(const MacroExpander* expander);

void pp_token_buffer_destroy(PPTokenBuffer* buffer);

#endif /* PREPROCESSOR_MACRO_EXPANDER_H */
