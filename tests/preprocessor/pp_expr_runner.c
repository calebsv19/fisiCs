#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "Lexer/lexer.h"
#include "Lexer/token_buffer.h"
#include "Preprocessor/macro_table.h"
#include "Preprocessor/pp_expr.h"
#include "Compiler/diagnostics.h"
#include "Compiler/frontend_limits.h"

bool compiler_report_diag(struct CompilerContext* ctx,
                          SourceRange loc,
                          DiagKind kind,
                          int code,
                          const char* hint,
                          const char* fmt,
                          ...) {
    (void)ctx;
    (void)loc;
    (void)kind;
    (void)code;
    (void)hint;
    (void)fmt;
    return true;
}

static bool eval_expr(MacroTable* table,
                      const char* expr,
                      int32_t expected,
                      bool shouldFail) {
    Lexer lexer;
    initLexer(&lexer, expr, "<expr>", false);

    TokenBuffer buffer;
    token_buffer_init(&buffer);
    if (!token_buffer_fill_from_lexer(&buffer, &lexer)) {
        fprintf(stderr, "Failed to lex expression: %s\n", expr);
        token_buffer_destroy(&buffer);
        destroyLexer(&lexer);
        return false;
    }

    PPExprEvaluator eval;
    pp_expr_evaluator_init(&eval, table);

    int32_t value = 0;
    bool ok = pp_expr_eval_range(&eval, &buffer, 0, buffer.count, &value);
    token_buffer_destroy(&buffer);
    destroyLexer(&lexer);

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

static char* nested_expr(char prefix, char suffix, size_t depth) {
    size_t length = depth + 1 + (suffix ? depth : 0);
    char* expr = malloc(length + 1);
    if (!expr) return NULL;
    memset(expr, prefix, depth);
    expr[depth] = '1';
    if (suffix) {
        memset(expr + depth + 1, suffix, depth);
    }
    expr[length] = '\0';
    return expr;
}

static char* flat_comment_expr(size_t count) {
    static const char block[] = "/*x*/ ";
    static const char line[] = "//x\n";
    size_t capacity = count * (sizeof(block) - 1) + 2;
    char* expr = malloc(capacity);
    if (!expr) return NULL;
    size_t length = 0;
    for (size_t i = 0; i < count; ++i) {
        const char* comment = (i & 1u) ? line : block;
        size_t commentLength = strlen(comment);
        memcpy(expr + length, comment, commentLength);
        length += commentLength;
    }
    expr[length++] = '1';
    expr[length] = '\0';
    return expr;
}

static char* nested_conditional_expr(size_t depth) {
    size_t length = (depth * 4) + 1;
    char* expr = malloc(length + 1);
    if (!expr) return NULL;
    size_t cursor = 0;
    for (size_t i = 0; i < depth; ++i) {
        expr[cursor++] = '1';
        expr[cursor++] = '?';
    }
    expr[cursor++] = '1';
    for (size_t i = 0; i < depth; ++i) {
        expr[cursor++] = ':';
        expr[cursor++] = '0';
    }
    expr[cursor] = '\0';
    return expr;
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

    char* comments = flat_comment_expr(10000);
    char* shallowGroups = nested_expr('(', ')', 32);
    char* deepGroups = nested_expr('(', ')', FISICS_FRONTEND_MAX_EXPRESSION_DEPTH + 1u);
    char* deepUnary = nested_expr('!', '\0', FISICS_FRONTEND_MAX_EXPRESSION_DEPTH + 1u);
    char* deepConditional = nested_conditional_expr(FISICS_FRONTEND_MAX_EXPRESSION_DEPTH + 1u);
    ok = ok && comments && eval_expr(table, comments, 1, false);
    ok = ok && shallowGroups && eval_expr(table, shallowGroups, 1, false);
    ok = ok && deepGroups && eval_expr(table, deepGroups, 0, true);
    ok = ok && deepUnary && eval_expr(table, deepUnary, 0, true);
    ok = ok && deepConditional && eval_expr(table, deepConditional, 0, true);
    free(comments);
    free(shallowGroups);
    free(deepGroups);
    free(deepUnary);
    free(deepConditional);

    macro_table_destroy(table);
    return ok ? 0 : 1;
}
