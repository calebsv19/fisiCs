// SPDX-License-Identifier: Apache-2.0

#ifndef PREPROCESSOR_INTERNAL_H
#define PREPROCESSOR_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>

#include "Compiler/diagnostics.h"
#include "Preprocessor/preprocessor.h"

// Shared conditional frame used across the driver and conditional handlers.
typedef struct {
    bool parentActive;
    bool selfActive;
    bool branchTaken;
    bool sawElse;
} PPConditionalFrame;

typedef struct PPIncludeFrame {
    const char* path;
    IncludeSearchOrigin origin;
    size_t originIndex;
} PPIncludeFrame;

// Utility helpers (defined in preprocessor_utils.c)
void pp_token_buffer_init_local(PPTokenBuffer* buffer);
bool pp_token_buffer_reserve(PPTokenBuffer* buffer, size_t extra);
char* pp_strdup_local(const char* s);
void pp_token_free(Token* tok);
Token pp_token_clone(const Token* tok);
Token pp_token_clone_remap(Preprocessor* pp, const Token* tok);
bool pp_token_buffer_append_token(PPTokenBuffer* buffer, Token token);
bool pp_token_buffer_append_clone(PPTokenBuffer* buffer, const Token* tok);
bool pp_token_buffer_append_clone_remap(PPTokenBuffer* buffer,
                                         Preprocessor* pp,
                                         const Token* tok);
bool pp_token_buffer_append_buffer(PPTokenBuffer* dest, const PPTokenBuffer* src);
void pp_token_buffer_reset(PPTokenBuffer* buffer);
void skip_to_line_end(const Token* tokens, size_t count, size_t* cursor);
bool pp_debug_fail_enabled(void);
void pp_debug_fail(const char* label, const Token* tok);
const char* token_file(const Token* tok);
bool pp_set_base_file(Preprocessor* pp, const char* path);
bool pp_set_logical_file(Preprocessor* pp, const char* path);
bool pp_append_number_literal(Preprocessor* pp,
                              PPTokenBuffer* buffer,
                              const Token* base,
                              const char* text);
bool pp_prepare_expr_tokens(Preprocessor* pp,
                            const Token* tokens,
                            size_t count,
                            PPTokenBuffer* out);
bool pp_include_stack_push(Preprocessor* pp,
                           const char* path,
                           IncludeSearchOrigin origin,
                           size_t originIndex);
void pp_include_stack_pop(Preprocessor* pp);
const PPIncludeFrame* pp_include_stack_top(const Preprocessor* pp);
bool pp_include_stack_contains(const Preprocessor* pp, const char* path);

void pp_report_diag(Preprocessor* pp,
                    const Token* tok,
                    DiagKind kind,
                    int code,
                    const char* fmt,
                    ...);

// Driver helper exposed so includes can recurse without appending EOF.
bool preprocess_tokens(Preprocessor* pp,
                       const TokenBuffer* input,
                       PPTokenBuffer* output,
                       bool appendEOF);

// Conditional handlers (defined in preprocessor_conditionals.c)
bool conditional_stack_is_active(const PPConditionalFrame* stack, size_t depth);
bool process_if(Preprocessor* pp,
                const Token* tokens,
                size_t count,
                size_t* cursor,
                PPConditionalFrame** stack,
                size_t* depth,
                size_t* capacity);
bool process_ifdeflike(Preprocessor* pp,
                       const Token* tokens,
                       size_t count,
                       size_t* cursor,
                       PPConditionalFrame** stack,
                       size_t* depth,
                       size_t* capacity,
                       bool negate);
bool process_elif(Preprocessor* pp,
                  const Token* tokens,
                  size_t count,
                  size_t* cursor,
                  PPConditionalFrame* stack,
                  size_t depth);
bool process_else(Preprocessor* pp, PPConditionalFrame* stack, size_t depth, const Token* tok);
bool process_endif(Preprocessor* pp, PPConditionalFrame* stack, size_t* depth, const Token* tok);

// Directive handlers (defined in preprocessor_directives.c)
bool process_define(Preprocessor* pp,
                    const Token* tokens,
                    size_t count,
                    size_t* cursor);
bool process_include(Preprocessor* pp,
                     const Token* tokens,
                     size_t count,
                     size_t* cursor,
                     PPTokenBuffer* output,
                     bool isIncludeNext);
bool process_undef(Preprocessor* pp,
                   const Token* tokens,
                   size_t count,
                   size_t* cursor);
bool process_pragma(Preprocessor* pp,
                    const Token* tokens,
                    size_t count,
                    size_t* cursor);
bool process_line_directive(Preprocessor* pp,
                            const Token* tokens,
                            size_t count,
                            size_t* cursor);

#endif /* PREPROCESSOR_INTERNAL_H */
