#ifndef DESIGNATED_INIT_H
#define DESIGNATED_INIT_H

#include "AST/ast_node.h"  // For ASTNode forward declaration
#include <stddef.h>
#include <stdbool.h>

struct ASTNode;

typedef struct DesignatedInit DesignatedInit;


struct DesignatedInit{
    char* fieldName;        // e.g., .field = ...
    struct ASTNode* indexExpr;
    struct ASTNode* expression;    // The expression assigned
};

// Create a designated initializer
DesignatedInit* createDesignatedInit(const char* fieldName, struct ASTNode* expression);
DesignatedInit* createSimpleInit(struct ASTNode* expr);
DesignatedInit* createIndexedInit(struct ASTNode* indexExpr, struct ASTNode* valueExpr);
struct ASTNode* createCompoundInit(struct DesignatedInit** entries, size_t count);


// Free function if needed later
void freeDesignatedInit(DesignatedInit* init);

#endif

