#include "Preprocessor/pp_internal.h"

#include <stdlib.h>

#include "Lexer/tokens.h"

bool conditional_stack_is_active(const PPConditionalFrame* stack, size_t depth) {
    if (depth == 0) return true;
    return stack[depth - 1].selfActive;
}

static bool token_is_macro_identifier(const Token* tok) {
    if (!tok || !tok->value) return false;
    if (tok->type == TOKEN_IDENTIFIER) return true;
    if (tok->type < TOKEN_IDENTIFIER) return true; // keyword tokens like __inline__
    return false;
}

static bool push_conditional(PPConditionalFrame** stack,
                             size_t* depth,
                             size_t* capacity,
                             PPConditionalFrame frame) {
    if (*depth == *capacity) {
        size_t newCap = (*capacity == 0) ? 4 : (*capacity * 2);
        PPConditionalFrame* newStack = realloc(*stack, newCap * sizeof(PPConditionalFrame));
        if (!newStack) return false;
        *stack = newStack;
        *capacity = newCap;
    }
    (*stack)[(*depth)++] = frame;
    return true;
}

bool process_if(Preprocessor* pp,
                const Token* tokens,
                size_t count,
                size_t* cursor,
                PPConditionalFrame** stack,
                size_t* depth,
                size_t* capacity) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    size_t exprStart = i + 1;
    size_t exprEnd = exprStart;
    while (exprEnd < count && tokens[exprEnd].type != TOKEN_EOF && tokens[exprEnd].line == directiveLine) {
        exprEnd++;
    }

    bool parentActive = conditional_stack_is_active(*stack, *depth);
    bool condValue = false;
    if (parentActive) {
        int32_t value = 0;
        PPTokenBuffer prepared = {0};
        const Token* evalTokens = tokens + exprStart;
        size_t evalCount = exprEnd - exprStart;
        if (!pp_prepare_expr_tokens(pp, evalTokens, evalCount, &prepared)) {
            pp_token_buffer_destroy(&prepared);
            return false;
        }
        if (prepared.count > 0) {
            evalTokens = prepared.tokens;
            evalCount = prepared.count;
        }
        preprocessor_eval_tokens(pp, evalTokens, evalCount, &value);
        condValue = (value != 0);
        pp_token_buffer_destroy(&prepared);
    }

    PPConditionalFrame frame;
    frame.parentActive = parentActive;
    frame.selfActive = parentActive && condValue;
    frame.branchTaken = frame.selfActive;
    frame.sawElse = false;

    if (!push_conditional(stack, depth, capacity, frame)) {
        return false;
    }

    *cursor = (exprEnd == 0) ? 0 : exprEnd - 1;
    return true;
}

bool process_ifdeflike(Preprocessor* pp,
                       const Token* tokens,
                       size_t count,
                       size_t* cursor,
                       PPConditionalFrame** stack,
                       size_t* depth,
                       size_t* capacity,
                       bool negate) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i >= count || !token_is_macro_identifier(&tokens[i])) {
        if (pp_debug_fail_enabled()) {
            fprintf(stderr, "[PP-DEBUG] ifdeflike name token type=%d val=%s\n",
                    (i < count) ? tokens[i].type : -1,
                    (i < count && tokens[i].value) ? tokens[i].value : "<null>");
        }
        pp_report_diag(pp, tokens ? &tokens[i] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "expected identifier after #%s",
                       negate ? "ifndef" : "ifdef");
        return false;
    }
    const char* name = tokens[i].value;
    bool parentActive = conditional_stack_is_active(*stack, *depth);
    bool isDefined = parentActive && (macro_table_lookup(pp->table, name) != NULL);
    bool selfActive = parentActive && (negate ? !isDefined : isDefined);

    PPConditionalFrame frame;
    frame.parentActive = parentActive;
    frame.selfActive = selfActive;
    frame.branchTaken = selfActive;
    frame.sawElse = false;

    if (!push_conditional(stack, depth, capacity, frame)) {
        return false;
    }

    // skip rest of the line
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
    return true;
}

bool process_elif(Preprocessor* pp,
                  const Token* tokens,
                  size_t count,
                  size_t* cursor,
                  PPConditionalFrame* stack,
                  size_t depth) {
    if (depth == 0) {
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "#elif without matching #if");
        return pp && pp->lenientMissingIncludes;
    }
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    size_t exprStart = i + 1;
    size_t exprEnd = exprStart;
    while (exprEnd < count && tokens[exprEnd].type != TOKEN_EOF && tokens[exprEnd].line == directiveLine) {
        exprEnd++;
    }

    PPConditionalFrame* frame = &stack[depth - 1];
    if (frame->sawElse) {
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "#elif after #else");
        return pp && pp->lenientMissingIncludes;
    }

    bool newActive = false;
    if (frame->parentActive && !frame->branchTaken) {
        int32_t value = 0;
        PPTokenBuffer prepared = {0};
        const Token* evalTokens = tokens + exprStart;
        size_t evalCount = exprEnd - exprStart;
        if (!pp_prepare_expr_tokens(pp, evalTokens, evalCount, &prepared)) {
            pp_token_buffer_destroy(&prepared);
            return false;
        }
        if (prepared.count > 0) {
            evalTokens = prepared.tokens;
            evalCount = prepared.count;
        }
        preprocessor_eval_tokens(pp, evalTokens, evalCount, &value);
        pp_token_buffer_destroy(&prepared);
        newActive = (value != 0);
    }
    frame->selfActive = frame->parentActive && newActive;
    if (newActive && frame->parentActive) {
        frame->branchTaken = true;
    }

    *cursor = (exprEnd == 0) ? 0 : exprEnd - 1;
    return true;
}

bool process_else(Preprocessor* pp, PPConditionalFrame* stack, size_t depth, const Token* tok) {
    if (depth == 0) {
        pp_report_diag(pp, tok, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "#else without matching #if");
        return pp && pp->lenientMissingIncludes;
    }
    PPConditionalFrame* frame = &stack[depth - 1];
    if (frame->sawElse) {
        pp_report_diag(pp, tok, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "duplicate #else");
        return pp && pp->lenientMissingIncludes;
    }
    bool newActive = frame->parentActive && !frame->branchTaken;
    frame->selfActive = newActive;
    if (newActive) {
        frame->branchTaken = true;
    }
    frame->sawElse = true;
    return true;
}

bool process_endif(Preprocessor* pp, PPConditionalFrame* stack, size_t* depth, const Token* tok) {
    (void)stack;
    if (*depth == 0) {
        pp_report_diag(pp, tok, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "#endif without matching #if");
        return pp && pp->lenientMissingIncludes;
    }
    (*depth)--;
    return true;
}
