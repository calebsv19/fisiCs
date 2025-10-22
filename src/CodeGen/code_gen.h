#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "AST/ast_node.h"
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

typedef struct CodegenContext CodegenContext;
struct SemanticModel;

CodegenContext* codegen_context_create(const char* moduleName, const struct SemanticModel* semanticModel);
void codegen_context_destroy(CodegenContext* ctx);

LLVMModuleRef codegen_get_module(const CodegenContext* ctx);

LLVMValueRef codegen_generate(CodegenContext* ctx, ASTNode* node);

#endif // CODE_GEN_H
