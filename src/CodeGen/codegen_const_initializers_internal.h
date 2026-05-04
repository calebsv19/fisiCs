// SPDX-License-Identifier: Apache-2.0

#ifndef CODEGEN_CONST_INITIALIZERS_INTERNAL_H
#define CODEGEN_CONST_INITIALIZERS_INTERNAL_H

#include "codegen_private.h"

const ParsedType* cg_resolve_typedef_parsed(CodegenContext* ctx, const ParsedType* type);
unsigned long long cg_eval_initializer_index_const(CodegenContext* ctx,
                                                   ASTNode* expr,
                                                   bool* outSuccess);

LLVMValueRef cg_build_const_array(CodegenContext* ctx,
                                  LLVMTypeRef arrayType,
                                  const ParsedType* parsedType,
                                  DesignatedInit** entries,
                                  size_t entryCount);
LLVMValueRef cg_build_const_struct(CodegenContext* ctx,
                                   LLVMTypeRef structType,
                                   const ParsedType* parsedType,
                                   DesignatedInit** entries,
                                   size_t entryCount);

#endif
