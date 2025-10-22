#include "semantic_pass.h"
#include "scope.h"
#include "syntax_errors.h"
#include "analyze_core.h"
#include "builtins.h"

void analyzeSemanticsWithContext(ASTNode* root, CompilerContext* ctx) {
    initErrorList();

    Scope* globalScope = createScope(NULL);
    globalScope->ctx = ctx;

    seedBuiltins(globalScope);

    if (!root || root->type != AST_PROGRAM) {
        addError(root ? root->line : 0, 0, "Root AST node is not a program", "Ensure parsing produced a valid program node");
        return;
    }

    // Call main recursive analyzer
    analyze(root, globalScope);

    reportErrors();
    if (getErrorCount() == 0) {
        printf("Semantic analysis: no issues found.\n");
    }
    destroyScope(globalScope);
    freeErrorList();
}

void analyzeSemantics(ASTNode* root) {
    CompilerContext* ctx = cc_create();
    cc_seed_builtins(ctx);
    analyzeSemanticsWithContext(root, ctx);
    cc_destroy(ctx);
}
