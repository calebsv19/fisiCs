#ifndef SEMANTIC_PASS_H
#define SEMANTIC_PASS_H


#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include <stdbool.h>

typedef struct SemanticModel SemanticModel;

SemanticModel* analyzeSemanticsBuildModel(ASTNode* root, CompilerContext* ctx, bool takeContextOwnership);

void analyzeSemantics(ASTNode* root);
void analyzeSemanticsWithContext(ASTNode* root, CompilerContext* ctx);

#endif // SEMANTIC_PASS_H
