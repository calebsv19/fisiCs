// SPDX-License-Identifier: Apache-2.0

#ifndef CODEGEN_INITIALIZERS_AGGREGATE_H
#define CODEGEN_INITIALIZERS_AGGREGATE_H

#include "codegen_private.h"

const CCTagFieldLayout* cg_init_lookup_field_layout(CodegenContext* ctx,
                                                    LLVMTypeRef aggregateType,
                                                    const ParsedType* aggregateParsed,
                                                    const char* fieldName);
bool cg_init_field_by_index(CodegenContext* ctx,
                            const ParsedType* aggregateParsed,
                            unsigned targetIndex,
                            const char** outFieldName,
                            const ParsedType** outParsed);
const StructInfo* cg_init_lookup_legacy_struct_info(CodegenContext* ctx,
                                                    const char* structName,
                                                    LLVMTypeRef aggregateType);
CGStructLLVMInfo* cg_init_find_struct_info_for_aggregate(CodegenContext* ctx,
                                                         LLVMTypeRef aggregateType,
                                                         const ParsedType* parsedHint);
bool cg_init_store_bitfield(CodegenContext* ctx,
                            LLVMValueRef basePtr,
                            const CCTagFieldLayout* lay,
                            LLVMValueRef value,
                            const ParsedType* fromParsed,
                            const ParsedType* toParsed);
unsigned long long cg_eval_initializer_index(ASTNode* expr, bool* outSuccess);
bool cg_entries_flat_scalars(DesignatedInit** entries, size_t entryCount);
bool cg_expr_initializes_whole_aggregate(CodegenContext* ctx,
                                         ASTNode* expr,
                                         LLVMTypeRef destType);
bool cg_entries_have_designators(DesignatedInit** entries, size_t entryCount);

#endif // CODEGEN_INITIALIZERS_AGGREGATE_H
