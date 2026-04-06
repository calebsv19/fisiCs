// SPDX-License-Identifier: Apache-2.0

#include "semantic_model_printer.h"

#include "semantic_model.h"
#include "scope.h"
#include "symbol_table.h"

#include <stdio.h>

static void printSymbol(const Symbol* sym) {
    const char* kindStr = "unknown";
    switch (sym->kind) {
        case SYMBOL_VARIABLE: kindStr = "variable"; break;
        case SYMBOL_FUNCTION: kindStr = "function"; break;
        case SYMBOL_TYPEDEF:  kindStr = "typedef";  break;
        case SYMBOL_STRUCT:   kindStr = "struct";   break;
        case SYMBOL_ENUM:     kindStr = "enum";     break;
        default: break;
    }

    printf("  - %s: %s\n", kindStr, sym->name);
}

static void dumpScope(const Scope* scope) {
    if (!scope) return;

    printf("Scope depth %d\n", scope->depth);

    for (int i = 0; i < SYMBOL_TABLE_SIZE; ++i) {
        Symbol* sym = scope->table.buckets[i];
        while (sym) {
            printSymbol(sym);
            sym = sym->next;
        }
    }
}

void semanticModelDump(const SemanticModel* model) {
    if (!model) {
        printf("<null semantic model>\n");
        return;
    }

    printf("SemanticModel: %zu error(s)\n", semanticModelGetErrorCount(model));

    const Scope* scope = semanticModelGetGlobalScope(model);
    dumpScope(scope);
}
