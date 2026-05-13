// SPDX-License-Identifier: Apache-2.0

#ifndef CODEGEN_DISPATCH_INTERNAL_H
#define CODEGEN_DISPATCH_INTERNAL_H

typedef struct ASTNode ASTNode;
typedef struct CodegenContext CodegenContext;
typedef struct LLVMOpaqueType* LLVMTypeRef;
typedef struct LLVMOpaqueValue* LLVMValueRef;

#ifdef __cplusplus
extern "C" {
#endif

LLVMValueRef codegenProgram(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenBlock(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenBinaryExpression(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenUnaryExpression(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenTernaryExpression(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenCommaExpression(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenAssignment(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenVariableDeclaration(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenArrayAccess(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenPointerAccess(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenDotAccess(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenPointerDereference(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenIfStatement(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenWhileLoop(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenForLoop(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenFunctionDefinition(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenFunctionCall(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenTypedef(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenEnumDefinition(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenCastExpression(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenCompoundLiteral(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenReturn(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenBreak(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenContinue(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenNumberLiteral(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenCharLiteral(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenStringLiteral(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenIdentifier(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenSizeof(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenAlignof(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenStatementExpression(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenSwitch(CodegenContext* ctx, ASTNode* node);
LLVMTypeRef codegenStructDefinition(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenStructFieldAccess(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenHeapAllocation(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenLabel(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenGoto(CodegenContext* ctx, ASTNode* node);

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_DISPATCH_INTERNAL_H
