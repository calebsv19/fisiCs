// SPDX-License-Identifier: Apache-2.0

#ifndef PREPROCESSOR_MACRO_EXPANDER_INTERNAL_H
#define PREPROCESSOR_MACRO_EXPANDER_INTERNAL_H

#include "Preprocessor/macro_expander.h"

typedef struct {
    PPTokenBuffer* items;
    size_t count;
    size_t capacity;
} PPArgumentList;

typedef struct {
    PPTokenBuffer raw;
    PPTokenBuffer expanded;
} MacroArg;

typedef struct {
    MacroArg* positionals;
    size_t positionalCount;
    MacroArg* variadics;
    size_t variadicCount;
} MacroArgPack;

SourceRange empty_range(void);
void macro_expander_clear_error(MacroExpander* expander);
void macro_expander_set_arg_count_error(MacroExpander* expander,
                                        const MacroDefinition* def,
                                        SourceRange callSite,
                                        size_t expectedArgs,
                                        size_t providedArgs);
void macro_expander_set_unsupported_gnu_comma_va_args_error(MacroExpander* expander,
                                                            const MacroDefinition* def,
                                                            SourceRange callSite);

char* pp_strdup(const char* s);
void token_free(Token* tok);
Token token_clone(const Token* tok);
bool range_is_initialized(const SourceRange* range);

bool pp_token_buffer_append(PPTokenBuffer* buffer, Token token);
bool pp_token_buffer_append_clone(PPTokenBuffer* buffer, const Token* tok);
void pp_token_buffer_move(PPTokenBuffer* dest, PPTokenBuffer* src);
bool pp_token_buffer_pop_if_type(PPTokenBuffer* buffer, TokenType type);
void pp_token_buffer_release(PPTokenBuffer* buffer);
void pp_token_buffer_destroy(PPTokenBuffer* buffer);

void pp_argument_list_init(PPArgumentList* list);
void pp_argument_list_destroy(PPArgumentList* list);

void macro_arg_init(MacroArg* arg);
void macro_arg_destroy(MacroArg* arg);
void macro_arg_pack_destroy(MacroArgPack* pack);

bool parse_macro_argument_tokens(const Token* input,
                                 size_t totalCount,
                                 size_t* cursor,
                                 PPArgumentList* outArgs,
                                 const MacroDefinition* def);
bool build_macro_arg_pack(MacroExpander* expander,
                          const MacroDefinition* def,
                          PPArgumentList* rawArgs,
                          MacroArgPack* pack,
                          SourceRange callSite);
bool expand_macro_arguments(MacroExpander* expander, MacroArgPack* pack);

#endif /* PREPROCESSOR_MACRO_EXPANDER_INTERNAL_H */
