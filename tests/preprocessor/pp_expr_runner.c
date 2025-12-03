#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "Lexer/lexer.h"
#include "Lexer/token_buffer.h"
#include "Preprocessor/macro_table.h"
#include "Preprocessor/pp_expr.h"

static bool eval_expr(MacroTable* table,
                      const char* expr,
                      int32_t expected,
                      bool shouldFail) {
    Lexer lexer;
    initLexer(&lexer, expr, "<expr>");

    TokenBuffer buffer;
    token_buffer_init(&buffer);
    if (!token_buffer_fill_from_lexer(&buffer, &lexer)) {
        fprintf(stderr, "Failed to lex expression: %s\n", expr);
        token_buffer_destroy(&buffer);
        return false;
    }

    PPExprEvaluator eval;
    pp_expr_evaluator_init(&eval, table);

    int32_t value = 0;
    bool ok = pp_expr_eval_range(&eval, &buffer, 0, buffer.count, &value);
    token_buffer_destroy(&buffer);

    if (shouldFail) {
        if (ok) {
            fprintf(stderr, "Expected failure for '%s' but got %d\n", expr, value);
            return false;
        }
        return true;
    }

    if (!ok) {
        fprintf(stderr, "Evaluation failed for '%s'\n", expr);
        return false;
    }
    if (value != expected) {
        fprintf(stderr, "Unexpected value for '%s': got %d, want %d\n", expr, value, expected);
        return false;
    }
    return true;
}

int main(void) {
    MacroTable* table = macro_table_create();
    if (!table) {
        fprintf(stderr, "OOM creating macro table\n");
        return 1;
    }

    SourceRange zero = {0};
    macro_table_define_object(table, "FOO", NULL, 0, zero);

    bool ok = true;
    ok = ok && eval_expr(table, "1 + 2 * 3", 7, false);
    ok = ok && eval_expr(table, "defined(FOO)", 1, false);
    ok = ok && eval_expr(table, "defined(BAR)", 0, false);
    ok = ok && eval_expr(table, "1 && defined(FOO)", 1, false);
    ok = ok && eval_expr(table, "0 && defined(FOO)", 0, false); // short-circuit
    ok = ok && eval_expr(table, "1 ? 4 : 5", 4, false);
    ok = ok && eval_expr(table, "0 ? 4 : 5", 5, false);
    ok = ok && eval_expr(table, "1 + 2 == 3", 1, false);
    ok = ok && eval_expr(table, "1 << 5", 32, false);
    ok = ok && eval_expr(table, "1 / 0", 0, true); // should fail

    macro_table_destroy(table);
    return ok ? 0 : 1;
}
