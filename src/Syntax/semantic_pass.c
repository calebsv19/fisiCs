#include "semantic_pass.h"
#include "scope.h"
#include "syntax_errors.h"
#include "analyze_core.h"

void analyzeSemantics(ASTNode* root) {
    initErrorList();

    Scope* globalScope = createScope(NULL);

    if (!root || root->type != AST_PROGRAM) {
        addError(0, 0, "Root AST node is not a program", "Ensure parsing produced a valid program node");
        return;
    }

    // Call main recursive analyzer
    analyze(root, globalScope);

    reportErrors();
    destroyScope(globalScope);
    freeErrorList();
}

