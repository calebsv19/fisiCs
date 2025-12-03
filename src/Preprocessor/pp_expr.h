#ifndef PREPROCESSOR_PP_EXPR_H
#define PREPROCESSOR_PP_EXPR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Lexer/token_buffer.h"
#include "Preprocessor/macro_table.h"

typedef struct {
    MacroTable* macros;
} PPExprEvaluator;

void pp_expr_evaluator_init(PPExprEvaluator* eval, MacroTable* table);
void pp_expr_evaluator_reset(PPExprEvaluator* eval);

bool pp_expr_eval_tokens(PPExprEvaluator* eval,
                         const Token* tokens,
                         size_t count,
                         int32_t* outValue);

bool pp_expr_eval_range(PPExprEvaluator* eval,
                        const TokenBuffer* buffer,
                        size_t start,
                        size_t end,
                        int32_t* outValue);

#endif /* PREPROCESSOR_PP_EXPR_H */
