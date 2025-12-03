#ifndef PREPROCESSOR_MACRO_TABLE_H
#define PREPROCESSOR_MACRO_TABLE_H

#include <stdbool.h>
#include <stddef.h>

#include "Lexer/tokens.h"

typedef enum {
    MACRO_OBJECT = 0,
    MACRO_FUNCTION = 1
} MacroKind;

typedef struct {
    char** names;
    size_t count;
    bool variadic;
    bool hasVaOpt;
} MacroParamList;

typedef struct {
    Token* tokens;
    size_t count;
} MacroBody;

typedef struct MacroDefinition {
    char* name;
    MacroKind kind;
    MacroParamList params;
    MacroBody body;
    SourceRange definitionRange;
} MacroDefinition;

typedef struct {
    const MacroDefinition* macro;
    SourceRange callSiteRange;
    SourceRange expansionRange;
} MacroExpansionFrame;

typedef struct MacroTable {
    MacroDefinition* macros;
    size_t macroCount;
    size_t macroCapacity;

    MacroExpansionFrame* expansionStack;
    size_t expansionCount;
    size_t expansionCapacity;
} MacroTable;

MacroTable* macro_table_create(void);
void macro_table_destroy(MacroTable* table);

const MacroDefinition* macro_table_lookup(const MacroTable* table, const char* name);
bool macro_table_define_object(MacroTable* table,
                               const char* name,
                               const Token* bodyTokens,
                               size_t bodyCount,
                               SourceRange definitionRange);
bool macro_table_define_function(MacroTable* table,
                                 const char* name,
                                 const char* const* params,
                                 size_t paramCount,
                                 bool variadic,
                                 bool hasVaOpt,
                                 const Token* bodyTokens,
                                 size_t bodyCount,
                                 SourceRange definitionRange);
bool macro_table_undef(MacroTable* table, const char* name);

bool macro_table_push_expansion(MacroTable* table,
                                const MacroDefinition* macro,
                                SourceRange callSite,
                                SourceRange expansionRange);
void macro_table_pop_expansion(MacroTable* table, const MacroDefinition* macro);
bool macro_table_is_expanding(const MacroTable* table, const char* name);
const MacroExpansionFrame* macro_table_current_expansion(const MacroTable* table,
                                                         size_t* outCount);

#endif /* PREPROCESSOR_MACRO_TABLE_H */
