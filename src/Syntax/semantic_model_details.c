#include "semantic_model_details.h"

#include "scope.h"
#include "symbol_table.h"
#include "AST/ast_node.h"

#include <string.h>

static bool isStructLikeSymbol(const Symbol* sym) {
    if (!sym) return false;
    if (sym->kind == SYMBOL_STRUCT) return true;
    if (sym->definition && sym->definition->type == AST_UNION_DEFINITION) return true;
    return false;
}

size_t semanticModelGetStructCount(const SemanticModel* model) {
    if (!model) return 0;
    size_t count = 0;
    Scope* scope = semanticModelGetGlobalScope(model);
    if (!scope) return 0;
    for (int i = 0; i < SYMBOL_TABLE_SIZE; ++i) {
        for (Symbol* sym = scope->table.buckets[i]; sym; sym = sym->next) {
            if (isStructLikeSymbol(sym)) {
                ++count;
            }
        }
    }
    return count;
}

const Symbol* semanticModelGetStructByIndex(const SemanticModel* model, size_t index) {
    if (!model) return NULL;
    Scope* scope = semanticModelGetGlobalScope(model);
    if (!scope) return NULL;
    size_t current = 0;
    for (int i = 0; i < SYMBOL_TABLE_SIZE; ++i) {
        for (Symbol* sym = scope->table.buckets[i]; sym; sym = sym->next) {
            if (isStructLikeSymbol(sym)) {
                if (current == index) {
                    return sym;
                }
                ++current;
            }
        }
    }
    return NULL;
}

const Symbol* semanticModelLookupStruct(const SemanticModel* model, const char* name) {
    if (!model || !name) return NULL;
    const Symbol* sym = semanticModelLookupGlobal(model, name);
    if (isStructLikeSymbol(sym)) {
        return sym;
    }
    return NULL;
}
