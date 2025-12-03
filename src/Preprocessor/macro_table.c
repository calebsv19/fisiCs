#include "Preprocessor/macro_table.h"

#include <stdlib.h>
#include <string.h>

static char* mt_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

static void macro_body_destroy(MacroBody* body) {
    if (!body || !body->tokens) return;
    for (size_t i = 0; i < body->count; ++i) {
        free(body->tokens[i].value);
        body->tokens[i].value = NULL;
    }
    free(body->tokens);
    body->tokens = NULL;
    body->count = 0;
}

static bool macro_body_copy(MacroBody* dest, const Token* src, size_t count) {
    macro_body_destroy(dest);
    if (!src || count == 0) {
        dest->tokens = NULL;
        dest->count = 0;
        return true;
    }

    Token* tokens = (Token*)calloc(count, sizeof(Token));
    if (!tokens) {
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        tokens[i] = src[i];
        if (src[i].value) {
            tokens[i].value = mt_strdup(src[i].value);
            if (!tokens[i].value) {
                for (size_t j = 0; j < i; ++j) {
                    free(tokens[j].value);
                }
                free(tokens);
                return false;
            }
        }
    }

    dest->tokens = tokens;
    dest->count = count;
    return true;
}

static void macro_params_destroy(MacroParamList* params) {
    if (!params || !params->names) return;
    for (size_t i = 0; i < params->count; ++i) {
        free(params->names[i]);
    }
    free(params->names);
    params->names = NULL;
    params->count = 0;
    params->variadic = false;
    params->hasVaOpt = false;
}

static bool macro_params_copy(MacroParamList* dest,
                              const char* const* names,
                              size_t count,
                              bool variadic,
                              bool hasVaOpt) {
    macro_params_destroy(dest);
    if (!names || count == 0) {
        dest->names = NULL;
        dest->count = 0;
        dest->variadic = variadic;
        dest->hasVaOpt = hasVaOpt;
        return true;
    }
    char** copy = (char**)calloc(count, sizeof(char*));
    if (!copy) {
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        copy[i] = mt_strdup(names[i]);
        if (!copy[i]) {
            for (size_t j = 0; j < i; ++j) {
                free(copy[j]);
            }
            free(copy);
            return false;
        }
    }
    dest->names = copy;
    dest->count = count;
    dest->variadic = variadic;
    dest->hasVaOpt = hasVaOpt;
    return true;
}

static void macro_definition_reset(MacroDefinition* def) {
    if (!def) return;
    free(def->name);
    def->name = NULL;
    macro_params_destroy(&def->params);
    macro_body_destroy(&def->body);
    memset(&def->definitionRange, 0, sizeof(SourceRange));
    def->kind = MACRO_OBJECT;
}

static MacroDefinition* macro_table_find_mut(MacroTable* table, const char* name) {
    if (!table || !name) return NULL;
    for (size_t i = 0; i < table->macroCount; ++i) {
        if (strcmp(table->macros[i].name, name) == 0) {
            return &table->macros[i];
        }
    }
    return NULL;
}

static bool macro_table_grow(MacroTable* table) {
    if (table->macroCount < table->macroCapacity) {
        return true;
    }
    size_t newCapacity = (table->macroCapacity == 0) ? 8 : table->macroCapacity * 2;
    MacroDefinition* newEntries = (MacroDefinition*)realloc(table->macros,
                                                           newCapacity * sizeof(MacroDefinition));
    if (!newEntries) return false;
    table->macros = newEntries;
    memset(table->macros + table->macroCapacity, 0,
           (newCapacity - table->macroCapacity) * sizeof(MacroDefinition));
    table->macroCapacity = newCapacity;
    return true;
}

MacroTable* macro_table_create(void) {
    MacroTable* table = (MacroTable*)calloc(1, sizeof(MacroTable));
    return table;
}

void macro_table_destroy(MacroTable* table) {
    if (!table) return;
    for (size_t i = 0; i < table->macroCount; ++i) {
        macro_definition_reset(&table->macros[i]);
    }
    free(table->macros);
    free(table->expansionStack);
    free(table);
}

const MacroDefinition* macro_table_lookup(const MacroTable* table, const char* name) {
    if (!table || !name) return NULL;
    for (size_t i = 0; i < table->macroCount; ++i) {
        if (strcmp(table->macros[i].name, name) == 0) {
            return &table->macros[i];
        }
    }
    return NULL;
}

static MacroDefinition* macro_table_upsert(MacroTable* table, const char* name) {
    MacroDefinition* existing = macro_table_find_mut(table, name);
    if (existing) {
        macro_definition_reset(existing);
        existing->name = mt_strdup(name);
        return existing;
    }
    if (!macro_table_grow(table)) {
        return NULL;
    }
    MacroDefinition* slot = &table->macros[table->macroCount++];
    memset(slot, 0, sizeof(*slot));
    slot->name = mt_strdup(name);
    if (!slot->name) {
        table->macroCount--;
        return NULL;
    }
    return slot;
}

bool macro_table_define_object(MacroTable* table,
                               const char* name,
                               const Token* bodyTokens,
                               size_t bodyCount,
                               SourceRange definitionRange) {
    if (!table || !name) return false;
    MacroDefinition* def = macro_table_upsert(table, name);
    if (!def) return false;
    def->kind = MACRO_OBJECT;
    if (!macro_body_copy(&def->body, bodyTokens, bodyCount)) {
        macro_definition_reset(def);
        return false;
    }
    def->definitionRange = definitionRange;
    return true;
}

bool macro_table_define_function(MacroTable* table,
                                 const char* name,
                                 const char* const* params,
                                 size_t paramCount,
                                 bool variadic,
                                 bool hasVaOpt,
                                 const Token* bodyTokens,
                                 size_t bodyCount,
                                 SourceRange definitionRange) {
    if (!table || !name) return false;
    MacroDefinition* def = macro_table_upsert(table, name);
    if (!def) return false;
    def->kind = MACRO_FUNCTION;
    if (!macro_params_copy(&def->params, params, paramCount, variadic, hasVaOpt)) {
        macro_definition_reset(def);
        return false;
    }
    if (!macro_body_copy(&def->body, bodyTokens, bodyCount)) {
        macro_definition_reset(def);
        return false;
    }
    def->definitionRange = definitionRange;
    return true;
}

bool macro_table_undef(MacroTable* table, const char* name) {
    if (!table || !name) return false;
    for (size_t i = 0; i < table->macroCount; ++i) {
        if (strcmp(table->macros[i].name, name) == 0) {
            macro_definition_reset(&table->macros[i]);
            table->macroCount--;
            if (i != table->macroCount) {
                table->macros[i] = table->macros[table->macroCount];
            }
            return true;
        }
    }
    return false;
}

static bool macro_expansion_grow(MacroTable* table) {
    if (table->expansionCount < table->expansionCapacity) {
        return true;
    }
    size_t newCapacity = (table->expansionCapacity == 0) ? 8 : table->expansionCapacity * 2;
    MacroExpansionFrame* frames =
        (MacroExpansionFrame*)realloc(table->expansionStack, newCapacity * sizeof(MacroExpansionFrame));
    if (!frames) return false;
    table->expansionStack = frames;
    table->expansionCapacity = newCapacity;
    return true;
}

bool macro_table_push_expansion(MacroTable* table,
                                const MacroDefinition* macro,
                                SourceRange callSite,
                                SourceRange expansionRange) {
    if (!table || !macro) return false;
    if (!macro_expansion_grow(table)) return false;
    MacroExpansionFrame* frame = &table->expansionStack[table->expansionCount++];
    frame->macro = macro;
    frame->callSiteRange = callSite;
    frame->expansionRange = expansionRange;
    return true;
}

void macro_table_pop_expansion(MacroTable* table, const MacroDefinition* macro) {
    if (!table || table->expansionCount == 0) return;
    MacroExpansionFrame* frame = &table->expansionStack[table->expansionCount - 1];
    if (macro && frame->macro != macro) {
        // Fallback: scan for the requested macro to keep stack consistent.
        for (size_t i = table->expansionCount; i > 0; --i) {
            if (table->expansionStack[i - 1].macro == macro) {
                table->expansionCount = i - 1;
                return;
            }
        }
    }
    if (table->expansionCount > 0) {
        table->expansionCount--;
    }
}

bool macro_table_is_expanding(const MacroTable* table, const char* name) {
    if (!table || !name) return false;
    for (size_t i = 0; i < table->expansionCount; ++i) {
        const MacroExpansionFrame* frame = &table->expansionStack[i];
        if (frame->macro && frame->macro->name && strcmp(frame->macro->name, name) == 0) {
            return true;
        }
    }
    return false;
}

const MacroExpansionFrame* macro_table_current_expansion(const MacroTable* table,
                                                         size_t* outCount) {
    if (outCount) {
        *outCount = table ? table->expansionCount : 0;
    }
    return table ? table->expansionStack : NULL;
}

MacroTable* macro_table_clone(const MacroTable* src) {
    if (!src) return NULL;
    MacroTable* clone = macro_table_create();
    if (!clone) return NULL;
    for (size_t i = 0; i < src->macroCount; ++i) {
        const MacroDefinition* def = &src->macros[i];
        if (def->kind == MACRO_OBJECT) {
            if (!macro_table_define_object(clone,
                                           def->name,
                                           def->body.tokens,
                                           def->body.count,
                                           def->definitionRange)) {
                macro_table_destroy(clone);
                return NULL;
            }
        } else {
            if (!macro_table_define_function(clone,
                                             def->name,
                                             (const char* const*)def->params.names,
                                             def->params.count,
                                             def->params.variadic,
                                             def->params.hasVaOpt,
                                             def->body.tokens,
                                             def->body.count,
                                             def->definitionRange)) {
                macro_table_destroy(clone);
                return NULL;
            }
        }
    }
    return clone;
}
