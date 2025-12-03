#ifndef PREPROCESSOR_DRIVER_H
#define PREPROCESSOR_DRIVER_H

#include <stdbool.h>

#include "Lexer/token_buffer.h"
#include "Preprocessor/macro_table.h"
#include "Preprocessor/macro_expander.h"

typedef struct {
    MacroTable* table;
    MacroExpander expander;
    bool preserveDirectives;
} Preprocessor;

bool preprocessor_init(Preprocessor* pp, bool preserveDirectives);
void preprocessor_destroy(Preprocessor* pp);

bool preprocessor_run(Preprocessor* pp,
                      const TokenBuffer* input,
                      PPTokenBuffer* output);

MacroTable* preprocessor_get_macro_table(Preprocessor* pp);

#endif /* PREPROCESSOR_DRIVER_H */
