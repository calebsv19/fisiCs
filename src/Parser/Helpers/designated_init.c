// SPDX-License-Identifier: Apache-2.0

#include "designated_init.h"
#include "AST/ast_node.h"
#include "Parser/Helpers/parsed_type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>  // Needed for 'bool'



static char* xstrdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}






DesignatedInit* createDesignatedInit(const char* fieldName, struct ASTNode* expression) {
    DesignatedInit* init = malloc(sizeof *init);
    if (!init) return NULL;
    init->fieldName = fieldName ? xstrdup(fieldName) : NULL;  // or strdup if you kept it
    init->indexExpr = NULL;                                   // IMPORTANT
    init->expression = expression;
    return init;
}

DesignatedInit* createSimpleInit(struct ASTNode* expr) {
    DesignatedInit* init = malloc(sizeof *init);
    if (!init) return NULL;
    init->fieldName = NULL;
    init->indexExpr = NULL;                                   // IMPORTANT
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
    ParsedType t;
    memset(&t, 0, sizeof t);
    t.kind = TYPE_INVALID;  // acts as "infer from LHS" in later analysis
    return createCompoundLiteralNode(t, entries, count);
}


void freeDesignatedInit(DesignatedInit* init) {
    if (init) {
        free(init->fieldName);
        // Expression is managed in AST, so we don’t free it here
        free(init);
    }
}


