#ifndef SEMANTIC_PASS_H
#define SEMANTIC_PASS_H


#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"

void analyzeSemantics(ASTNode* root);
void analyzeSemanticsWithContext(ASTNode* root, CompilerContext* ctx);

#endif // SEMANTIC_PASS_H
