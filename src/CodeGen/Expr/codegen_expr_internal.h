// SPDX-License-Identifier: Apache-2.0

#ifndef CODEGEN_EXPR_INTERNAL_H
#define CODEGEN_EXPR_INTERNAL_H

#include "codegen_private.h"
#include "codegen_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool cg_is_builtin_bool_literal_name(const char* name);
bool cg_parsed_type_is_pointerish(const ParsedType* t);
bool cg_eval_builtin_offsetof(CodegenContext* ctx,
                              const ParsedType* type,
                              const char* fieldName,
                              size_t* offsetOut);
bool cg_parsed_type_is_complex_value(const ParsedType* t);
bool cg_llvm_type_is_complex_value(LLVMTypeRef ty);
LLVMTypeRef cg_complex_element_type_from_llvm(LLVMTypeRef complexType);
LLVMTypeRef cg_complex_element_type(CodegenContext* ctx, const ParsedType* type);
LLVMValueRef cg_build_complex_value(CodegenContext* ctx,
                                    LLVMValueRef real,
                                    LLVMValueRef imag,
                                    LLVMTypeRef complexType,
                                    const char* nameHint);
LLVMValueRef cg_promote_to_complex(CodegenContext* ctx,
                                   LLVMValueRef value,
                                   const ParsedType* parsed,
                                   LLVMTypeRef complexType,
                                   LLVMTypeRef elemType);
void cg_promote_integer_operands(CodegenContext* ctx,
                                 LLVMValueRef* lhsValue,
                                 LLVMValueRef* rhsValue,
                                 LLVMTypeRef* lhsType,
                                 LLVMTypeRef* rhsType,
                                 bool lhsUnsigned,
                                 bool rhsUnsigned);
bool cg_is_unsigned_parsed(const ParsedType* type, LLVMTypeRef llvmType);
LLVMAtomicOrdering cg_atomic_order_from_builtin_arg(LLVMValueRef orderArg,
                                                    LLVMAtomicOrdering fallback,
                                                    bool forLoad,
                                                    bool forStore);
LLVMValueRef cg_atomic_cast_value(CodegenContext* ctx,
                                  LLVMValueRef value,
                                  LLVMTypeRef targetType,
                                  ASTNode* valueNode,
                                  const char* nameHint);
LLVMValueRef cg_atomic_cast_call_result(CodegenContext* ctx,
                                        ASTNode* callNode,
                                        LLVMValueRef value,
                                        const char* nameHint);
LLVMValueRef cg_atomic_cast_pointer(CodegenContext* ctx,
                                    LLVMValueRef pointerValue,
                                    LLVMTypeRef elementType,
                                    const char* nameHint);
LLVMValueRef cg_build_complex_addsub(CodegenContext* ctx,
                                     LLVMValueRef lhs,
                                     LLVMValueRef rhs,
                                     LLVMTypeRef complexType,
                                     bool isSub);
LLVMValueRef cg_build_complex_muldiv(CodegenContext* ctx,
                                     LLVMValueRef lhs,
                                     LLVMValueRef rhs,
                                     LLVMTypeRef complexType,
                                     bool isDiv);
LLVMValueRef cg_build_complex_eq(CodegenContext* ctx,
                                 LLVMValueRef lhs,
                                 LLVMValueRef rhs,
                                 bool isNotEqual);
LLVMValueRef cg_load_bitfield(CodegenContext* ctx,
                              const CGLValueInfo* info,
                              LLVMTypeRef resultType);
bool cg_store_bitfield(CodegenContext* ctx,
                       const CGLValueInfo* info,
                       LLVMValueRef value);
LLVMValueRef ensureIntegerLike(CodegenContext* ctx, LLVMValueRef value);
bool cg_is_float_type(LLVMTypeRef type);
unsigned cg_float_rank_from_kind(LLVMTypeKind kind);
bool cg_is_external_decl_function(LLVMValueRef function);
bool cg_is_known_external_abi_function_name(const char* name);
LLVMTypeRef cg_external_abi_coerce_param_type(CodegenContext* ctx, LLVMTypeRef paramType);
bool cg_should_lower_indirect_aggregate_return(CodegenContext* ctx, LLVMTypeRef returnType);
LLVMValueRef cg_pack_aggregate_for_external_abi(CodegenContext* ctx,
                                                LLVMValueRef value,
                                                LLVMTypeRef packedType,
                                                const char* nameHint);
LLVMTypeRef cg_select_float_type(LLVMTypeRef lhs, LLVMTypeRef rhs, LLVMContextRef ctx);
LLVMTypeRef vlaInnermostElementLLVM(CodegenContext* ctx, const ParsedType* type);
LLVMValueRef computeVLAElementCount(CodegenContext* ctx, const ParsedType* type);
ParsedType functionReturnTypeAtDerivation(const ParsedType* type, size_t functionIndex);
LLVMTypeRef functionTypeFromPointerParsed(CodegenContext* ctx,
                                          const ParsedType* type,
                                          size_t fallbackArgCount,
                                          LLVMValueRef* args);
bool cg_function_signatures_compatible(const Symbol* lhs, const Symbol* rhs);
const Symbol* cg_lookup_function_symbol_for_callee(CodegenContext* ctx, ASTNode* callee);
const Symbol* cg_lookup_global_function_symbol_by_name(CodegenContext* ctx, const char* name);
LLVMTypeRef cg_function_type_from_symbol(CodegenContext* ctx, const Symbol* sym);
unsigned cg_default_int_bits(CodegenContext* ctx);
LLVMValueRef cg_apply_default_promotion(CodegenContext* ctx,
                                        LLVMValueRef arg,
                                        ASTNode* argNode);
LLVMValueRef cg_prepare_call_argument(CodegenContext* ctx,
                                      LLVMValueRef arg,
                                      LLVMTypeRef paramTy,
                                      const ParsedType* fromParsed,
                                      const ParsedType* toParsed,
                                      const char* castName);
LLVMValueRef cg_emit_va_intrinsic(CodegenContext* ctx,
                                  const char* intrinsicName,
                                  LLVMValueRef listValue);
LLVMValueRef cg_emit_rewritten_builtin_call(CodegenContext* ctx,
                                            ASTNode* callNode,
                                            const char* targetName,
                                            LLVMTypeRef returnType,
                                            LLVMValueRef* sourceArgs,
                                            size_t sourceCount,
                                            const size_t* keepIndices,
                                            size_t keepCount,
                                            unsigned fixedParamCount,
                                            const LLVMTypeRef* fixedParamTypes,
                                            const bool* fixedParamSigned,
                                            bool isVariadic,
                                            const char* callName);
bool cg_try_codegen_builtin_call(CodegenContext* ctx,
                                 ASTNode* node,
                                 const char* calleeName,
                                 LLVMValueRef* args,
                                 LLVMTypeRef i8PtrType,
                                 LLVMTypeRef sizeType,
                                 LLVMTypeRef intType,
                                 LLVMValueRef* resultOut);

#ifdef __cplusplus
}
#endif

#endif /* CODEGEN_EXPR_INTERNAL_H */
