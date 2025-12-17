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
} PPBuiltinState;

typedef struct {
    MacroTable* table;
    PPBuiltinState builtins;
} MacroExpander;

void macro_expander_init(MacroExpander* expander, MacroTable* table);
void macro_expander_reset(MacroExpander* expander);
void macro_expander_set_builtins(MacroExpander* expander, PPBuiltinState state);

bool macro_expander_expand(MacroExpander* expander,
                           const Token* input,
                           size_t count,
                           PPTokenBuffer* outTokens);

void pp_token_buffer_destroy(PPTokenBuffer* buffer);

#endif /* PREPROCESSOR_MACRO_EXPANDER_H */
