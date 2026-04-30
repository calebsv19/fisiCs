// SPDX-License-Identifier: Apache-2.0

#include "semantic_model_printer.h"

#include "Extensions/extension_units_expr_table.h"
#include "Extensions/extension_units_view.h"
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

    const FisicsUnitsAnnotation* unitsAnn = symbolGetUnitsAnnotation(sym);
    if (unitsAnn) {
        const char* text = unitsAnn->canonicalText ? unitsAnn->canonicalText
                                                   : (unitsAnn->exprText ? unitsAnn->exprText : "<missing>");
        printf("  - %s: %s [units=%s", kindStr, sym->name, text);
        printf(", %s", unitsAnn->resolved ? "resolved" : "pending");
        if (unitsAnn->duplicateCount > 1) {
            printf(", duplicates=%zu", unitsAnn->duplicateCount);
        }
        printf(", decl-index=%zu]\n", symbolGetUnitsDeclaratorIndex(sym));
        return;
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
    fisics_extension_dump_units_annotations(semanticModelGetContext(model), stdout);
    fisics_extension_dump_units_expr_results(semanticModelGetContext(model), stdout);
    fflush(stdout);
}
