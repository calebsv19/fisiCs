#ifndef CODE_GEN_H
#define CODE_GEN_H



#include "Parser/designated_init.h"
#include "Parser/parsed_type.h"
#include "AST/ast_node.h"
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

// Initialize LLVM components
void initializeLLVM(void);

extern LLVMModuleRef TheModule;


// Generates LLVM IR from an AST node
LLVMValueRef codegen(ASTNode *node);

// Codegen functions for each AST node type
LLVMValueRef codegenProgram(ASTNode *node);
LLVMValueRef codegenBlock(ASTNode *node);
LLVMTypeRef codegenStructDefinition(ASTNode *node);

LLVMValueRef codegenBinaryExpression(ASTNode *node);
LLVMValueRef codegenUnaryExpression(ASTNode *node);
LLVMValueRef codegenTernaryExpression(ASTNode *node);
LLVMValueRef codegenAssignment(ASTNode *node);
LLVMValueRef codegenVariableDeclaration(ASTNode *node);
LLVMValueRef codegenArrayDeclaration(ASTNode *node);
LLVMValueRef codegenArrayAccess(ASTNode *node);
LLVMValueRef codegenPointerAccess(ASTNode *node);
LLVMValueRef codegenDotAccess(ASTNode *node);
LLVMValueRef codegenPointerDereference(ASTNode *node);
LLVMValueRef codegenIfStatement(ASTNode *node);
LLVMValueRef codegenWhileLoop(ASTNode *node);
LLVMValueRef codegenForLoop(ASTNode *node);
LLVMValueRef codegenFunctionDefinition(ASTNode *node);
LLVMValueRef codegenFunctionCall(ASTNode *node);
LLVMValueRef codegenReturn(ASTNode *node);
LLVMValueRef codegenBreak(ASTNode *node);
LLVMValueRef codegenContinue(ASTNode *node);
LLVMValueRef codegenNumberLiteral(ASTNode *node);
LLVMValueRef codegenCharLiteral(ASTNode *node);
LLVMValueRef codegenStringLiteral(ASTNode *node);
LLVMValueRef codegenIdentifier(ASTNode *node);
LLVMValueRef codegenSizeof(ASTNode *node);
LLVMValueRef codegenSwitch(ASTNode *node);
void codegenCase(ASTNode *node, LLVMValueRef switchInst, LLVMBasicBlockRef switchEnd);

// Helper Methods
int getFieldIndexFromStruct(LLVMValueRef structVal, const char* fieldName);



#endif // CODE_GEN_H

