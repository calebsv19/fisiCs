#include "designated_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>  // Needed for 'bool'


DesignatedInit* createDesignatedInit(const char* fieldName, struct ASTNode* expression) {
    DesignatedInit* init = malloc(sizeof(DesignatedInit));
    if (!init) {
        fprintf(stderr, "Error: Failed to allocate DesignatedInit\n");
        return NULL;
    }
    init->fieldName = strdup(fieldName);
    init->expression = expression;
    return init;
}

DesignatedInit* createSimpleInit(struct ASTNode* expr) {
    DesignatedInit* init = malloc(sizeof(DesignatedInit));
    init->fieldName = NULL;
    init->expression = expr;
    return init;
}

DesignatedInit* createIndexedInit(struct ASTNode* indexExpr, struct ASTNode* valueExpr) {
    if (!valueExpr) return NULL;

    DesignatedInit* init = malloc(sizeof(DesignatedInit));
    if (!init) {
        fprintf(stderr, "Error: Failed to allocate DesignatedInit for indexed init\n");
        return NULL;
    }

    init->fieldName = NULL;            // Not a struct field
    init->indexExpr = indexExpr;       // Index (e.g., 2)
    init->expression = valueExpr;      // Value (e.g., 99)

    return init;
}


ASTNode* createCompoundInit(DesignatedInit** entries, size_t count) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for compound initializer node\n");
        return NULL;
    }

    node->type = AST_COMPOUND_LITERAL;
    node->compoundLiteral.entries = entries;
    node->compoundLiteral.entryCount = count;
    node->nextStmt = NULL;

    return node;
}


void freeDesignatedInit(DesignatedInit* init) {
    if (init) {
        free(init->fieldName);
        // Expression is managed in AST, so we don’t free it here
        free(init);
    }
}

