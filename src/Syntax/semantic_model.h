#ifndef SEMANTIC_MODEL_H
#define SEMANTIC_MODEL_H

#include <stddef.h>
#include <stdbool.h>

#include "symbol_table.h"
#include "Compiler/compiler_context.h"
#include "Preprocessor/macro_table.h"

struct Scope;

typedef struct SemanticModel SemanticModel;
typedef void (*SemanticSymbolCallback)(const Symbol* symbol, void* userData);

SemanticModel* semanticModelCreate(struct Scope* globalScope,
                                   CompilerContext* ctx,
                                   bool ownsContext,
                                   size_t errorCount,
                                   MacroTable* macros,
                                   bool ownsMacros);
void semanticModelDestroy(SemanticModel* model);

size_t semanticModelGetErrorCount(const SemanticModel* model);
CompilerContext* semanticModelGetContext(const SemanticModel* model);
struct Scope* semanticModelGetGlobalScope(const SemanticModel* model);

void semanticModelForEachGlobal(const SemanticModel* model,
                                SemanticSymbolCallback callback,
                                void* userData);
const Symbol* semanticModelLookupGlobal(const SemanticModel* model, const char* name);
const MacroTable* semanticModelGetMacros(const SemanticModel* model);

#endif // SEMANTIC_MODEL_H
