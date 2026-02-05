#include "semantic_pass.h"
#include "semantic_model.h"
#include "scope.h"
#include "syntax_errors.h"
#include "analyze_core.h"
#include "builtins.h"
#include "control_flow.h"

#include <stdio.h>

#include <stdbool.h>

static Scope* runSemanticAnalysis(ASTNode* root,
                                  CompilerContext* ctx,
                                  size_t* outErrorCount,
                                  bool printSummary) {
    initErrorList(ctx);

    Scope* globalScope = createScope(NULL);
    if (!globalScope) {
        if (outErrorCount) *outErrorCount = 1;
        freeErrorList();
        return NULL;
    }
    globalScope->ctx = ctx;

    seedBuiltins(globalScope);

    if (!root || root->type != AST_PROGRAM) {
        addError(root ? root->line : 0, 0, "Root AST node is not a program", "Ensure parsing produced a valid program node");
    } else {
        analyze(root, globalScope);
        analyzeControlFlow(root, globalScope);
    }

    // Tentative definitions: promote remaining external tentative declarations to definitions
    SymbolTable* table = &globalScope->table;
    for (int i = 0; i < SYMBOL_TABLE_SIZE; ++i) {
        Symbol* sym = table->buckets[i];
        while (sym) {
            if (sym->kind == SYMBOL_VARIABLE &&
                sym->linkage == LINKAGE_EXTERNAL &&
                sym->isTentative &&
                !sym->hasDefinition) {
                sym->hasDefinition = true;
            }
            sym = sym->next;
        }
    }

    reportErrors();
    size_t errors = getErrorCount();
    if (printSummary && errors == 0) {
        printf("Semantic analysis: no issues found.\n");
    }
    if (outErrorCount) *outErrorCount = errors;
    freeErrorList();

    return globalScope;
}

void analyzeSemanticsWithContext(ASTNode* root, CompilerContext* ctx) {
    size_t errorCount = 0;
    Scope* globalScope = runSemanticAnalysis(root, ctx, &errorCount, true);
    if (globalScope) {
        destroyScope(globalScope);
    }
}

void analyzeSemantics(ASTNode* root) {
    CompilerContext* ctx = cc_create();
    cc_seed_builtins(ctx);
    analyzeSemanticsWithContext(root, ctx);
    cc_destroy(ctx);
}

SemanticModel* analyzeSemanticsBuildModel(ASTNode* root,
                                          CompilerContext* ctx,
                                          bool takeContextOwnership,
                                          MacroTable* macros,
                                          bool takeMacroOwnership) {
    size_t errorCount = 0;
    Scope* globalScope = runSemanticAnalysis(root, ctx, &errorCount, true);
    if (!globalScope) {
        if (takeContextOwnership && ctx) {
            cc_destroy(ctx);
        }
        if (takeMacroOwnership && macros) {
            macro_table_destroy(macros);
        }
        return NULL;
    }

    SemanticModel* model = semanticModelCreate(globalScope,
                                               ctx,
                                               takeContextOwnership,
                                               errorCount,
                                               macros,
                                               takeMacroOwnership);
    if (!model) {
        destroyScope(globalScope);
        if (takeContextOwnership && ctx) {
            cc_destroy(ctx);
        }
        if (takeMacroOwnership && macros) {
            macro_table_destroy(macros);
        }
    }
    return model;
}
