#ifndef CODEGEN_INTERNAL_H
#define CODEGEN_INTERNAL_H

#include "code_gen.h"
#include "codegen_type_cache.h"

#include <llvm-c/Core.h>

struct SemanticModel;
struct CGTypeCache;

#ifdef __cplusplus
extern "C" {
#endif

LLVMContextRef cg_context_get_llvm_context(CodegenContext* ctx);
LLVMModuleRef cg_context_get_module(CodegenContext* ctx);
LLVMBuilderRef cg_context_get_builder(CodegenContext* ctx);
const struct SemanticModel* cg_context_get_semantic_model(CodegenContext* ctx);
CGTypeCache* cg_context_get_type_cache(CodegenContext* ctx);
const TargetLayout* cg_context_get_target_layout(CodegenContext* ctx);

LLVMTypeRef cg_context_lookup_named_type(CodegenContext* ctx, const char* name);
void cg_context_cache_named_type(CodegenContext* ctx, const char* name, LLVMTypeRef type);
LLVMTypeRef cg_dereference_ptr_type(CodegenContext* ctx, LLVMTypeRef ptrType, const char* ctxMsg);

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_INTERNAL_H
