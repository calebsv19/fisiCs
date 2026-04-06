// SPDX-License-Identifier: Apache-2.0

#include "semantic_model.h"

#include "scope.h"

#include <stdlib.h>

struct SemanticModel {
    Scope* globalScope;
    CompilerContext* ctx;
    bool ownsContext;
    size_t errorCount;
    MacroTable* macros;
    bool ownsMacros;
};

SemanticModel* semanticModelCreate(Scope* globalScope,
                                   CompilerContext* ctx,
                                   bool ownsContext,
                                   size_t errorCount,
                                   MacroTable* macros,
                                   bool ownsMacros) {
    if (!globalScope) return NULL;
    SemanticModel* model = (SemanticModel*)calloc(1, sizeof(SemanticModel));
    if (!model) return NULL;
    model->globalScope = globalScope;
    model->ctx = ctx;
    model->ownsContext = ownsContext;
    model->errorCount = errorCount;
    model->macros = macros;
    model->ownsMacros = ownsMacros;
    return model;
}

void semanticModelDestroy(SemanticModel* model) {
    if (!model) return;
    destroyScope(model->globalScope);
    model->globalScope = NULL;
    if (model->ownsContext && model->ctx) {
        cc_destroy(model->ctx);
    }
    model->ctx = NULL;
    if (model->ownsMacros && model->macros) {
        macro_table_destroy(model->macros);
    }
    model->macros = NULL;
    free(model);
}

size_t semanticModelGetErrorCount(const SemanticModel* model) {
    return model ? model->errorCount : 0;
}

CompilerContext* semanticModelGetContext(const SemanticModel* model) {
    return model ? model->ctx : NULL;
}

Scope* semanticModelGetGlobalScope(const SemanticModel* model) {
    return model ? model->globalScope : NULL;
}

const MacroTable* semanticModelGetMacros(const SemanticModel* model) {
    return model ? model->macros : NULL;
}

void semanticModelForEachGlobal(const SemanticModel* model,
                                SemanticSymbolCallback callback,
                                void* userData) {
    if (!model || !model->globalScope || !callback) return;
    SymbolTable* table = &model->globalScope->table;
    for (int i = 0; i < SYMBOL_TABLE_SIZE; ++i) {
        Symbol* sym = table->buckets[i];
        while (sym) {
            callback(sym, userData);
            sym = sym->next;
        }
    }
}

const Symbol* semanticModelLookupGlobal(const SemanticModel* model, const char* name) {
    if (!model || !model->globalScope || !name) return NULL;
    return lookupSymbol(&model->globalScope->table, name);
}
