// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include "codegen_initializers_aggregate.h"
#include "codegen_types.h"
#include "Compiler/compiler_context.h"
#include "Syntax/layout.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <llvm-c/Target.h>

static bool cg_zero_initialize_storage(CodegenContext* ctx,
                                       LLVMValueRef destPtr,
                                       LLVMTypeRef destType,
                                       const ParsedType* destParsed) {
    if (!ctx || !destPtr || !destType || LLVMGetTypeKind(destType) == LLVMVoidTypeKind) {
        return false;
    }

    uint64_t bytes = 0;
    uint32_t align = 0;
    LLVMTargetDataRef td = ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
    if (!cg_size_align_for_type(ctx, destParsed, destType, &bytes, &align) || bytes == 0) {
        if (td) {
            bytes = LLVMABISizeOfType(td, destType);
            align = (uint32_t)LLVMABIAlignmentOfType(td, destType);
        }
    } else if (td) {
        uint64_t abiBytes = LLVMABISizeOfType(td, destType);
        uint32_t abiAlign = (uint32_t)LLVMABIAlignmentOfType(td, destType);
        if (abiBytes > bytes) {
            bytes = abiBytes;
        }
        if (abiAlign > align) {
            align = abiAlign;
        }
    }
    if (bytes == 0) {
        fprintf(stderr, "Error: Unable to determine aggregate initializer size\n");
        return false;
    }

    LLVMTypeRef i8Ptr = LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
    LLVMValueRef dstCast = LLVMBuildBitCast(ctx->builder, destPtr, i8Ptr, "init.zero.dst");
    LLVMValueRef sizeVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), bytes, 0);
    LLVMBuildMemSet(ctx->builder,
                    dstCast,
                    LLVMConstInt(LLVMInt8TypeInContext(ctx->llvmContext), 0, 0),
                    sizeVal,
                    align ? align : 1);
    return true;
}

bool cg_store_initializer_expression(CodegenContext* ctx,
                                     LLVMValueRef destPtr,
                                     LLVMTypeRef destType,
                                     const ParsedType* destParsed,
                                     ASTNode* expr) {
#define CG_STORE_INIT_RETURN(value) \
    do {                            \
        profiler_end(scope);        \
        return (value);             \
    } while (0)
    ProfilerScope scope = profiler_begin("codegen_store_initializer_expression");
    profiler_record_value("codegen_count_store_initializer_expression", 1);
    if (!ctx || !destPtr || !expr) CG_STORE_INIT_RETURN(false);
    LLVMTypeRef storeType = destType;
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        fprintf(stderr, "Error: Invalid initializer destination type\n");
        CG_STORE_INIT_RETURN(false);
    }

    LLVMTypeKind storeKind = LLVMGetTypeKind(storeType);
    bool isAggregateStore = (storeKind == LLVMStructTypeKind || storeKind == LLVMArrayTypeKind);

    // Zero initializer to memset if possible
    if (expr->type == AST_NUMBER_LITERAL && expr->valueNode.value && strcmp(expr->valueNode.value, "0") == 0) {
        if (!cg_zero_initialize_storage(ctx, destPtr, storeType, destParsed)) {
            CG_STORE_INIT_RETURN(false);
        }
        CG_STORE_INIT_RETURN(true);
    }

    if (isAggregateStore) {
        const ParsedType* callReturnParsed = cg_resolve_expression_type(ctx, expr);
        LLVMTypeRef callReturnType = callReturnParsed ? cg_type_from_parsed(ctx, callReturnParsed) : NULL;
        if (callReturnType && LLVMGetTypeKind(callReturnType) == LLVMFunctionTypeKind) {
            callReturnType = LLVMPointerType(callReturnType, 0);
        } else if (callReturnType && LLVMGetTypeKind(callReturnType) == LLVMArrayTypeKind) {
            callReturnType = LLVMPointerType(callReturnType, 0);
        }
        if (expr->type == AST_FUNCTION_CALL &&
            callReturnType &&
            callReturnType == storeType &&
            cg_should_lower_indirect_aggregate_return(ctx, callReturnType) &&
            cg_aggregate_type_contains_union(ctx, callReturnParsed, callReturnType)) {
            LLVMValueRef previousDestPtr = ctx->aggregateCallResultDestPtr;
            LLVMTypeRef previousDestType = ctx->aggregateCallResultDestType;
            ASTNode* previousDestCall = ctx->aggregateCallResultDestCall;
            ctx->aggregateCallResultDestPtr = destPtr;
            ctx->aggregateCallResultDestType = storeType;
            ctx->aggregateCallResultDestCall = expr;
            LLVMValueRef directResult = codegenNode(ctx, expr);
            ctx->aggregateCallResultDestPtr = previousDestPtr;
            ctx->aggregateCallResultDestType = previousDestType;
            ctx->aggregateCallResultDestCall = previousDestCall;
            if (directResult == destPtr) {
                CG_STORE_INIT_RETURN(true);
            }
            if (!directResult) {
                fprintf(stderr, "Error: Failed to evaluate aggregate initializer call\n");
                CG_STORE_INIT_RETURN(false);
            }
        }

        LLVMValueRef srcPtr = NULL;
        LLVMTypeRef srcType = NULL;
        const ParsedType* srcParsed = NULL;
        if (codegenLValue(ctx, expr, &srcPtr, &srcType, &srcParsed, NULL) && srcPtr) {
            uint64_t bytes = 0;
            uint32_t align = 0;
            if (!cg_size_align_for_type(ctx, destParsed, storeType, &bytes, &align) || bytes == 0) {
                LLVMTargetDataRef td = ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
                if (td) {
                    bytes = LLVMABISizeOfType(td, storeType);
                    align = (uint32_t)LLVMABIAlignmentOfType(td, storeType);
                }
            }
            if (bytes == 0) {
                fprintf(stderr, "Error: Unable to determine aggregate initializer size\n");
                CG_STORE_INIT_RETURN(false);
            }
            unsigned alignVal = align ? align : 1;
            LLVMTypeRef i8Ptr = LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
            LLVMValueRef dstCast = LLVMBuildBitCast(ctx->builder, destPtr, i8Ptr, "init.agg.dst");
            LLVMValueRef srcCast = LLVMBuildBitCast(ctx->builder, srcPtr, i8Ptr, "init.agg.src");
            LLVMValueRef sizeVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), bytes, 0);
            LLVMBuildMemCpy(ctx->builder, dstCast, alignVal, srcCast, alignVal, sizeVal);
            CG_STORE_INIT_RETURN(true);
        }
    }

    LLVMValueRef value = codegenNode(ctx, expr);
    if (!value) {
        fprintf(stderr, "Error: Failed to evaluate initializer expression\n");
        CG_STORE_INIT_RETURN(false);
    }

    if (!destType || LLVMGetTypeKind(destType) == LLVMVoidTypeKind) {
        storeType = LLVMTypeOf(value);
    }
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        fprintf(stderr, "Error: Invalid initializer destination type\n");
        CG_STORE_INIT_RETURN(false);
    }

    if (storeKind == LLVMArrayTypeKind && expr->type == AST_STRING_LITERAL) {
        LLVMValueRef constArray = cg_build_const_initializer(ctx, expr, storeType, destParsed);
        if (constArray && LLVMTypeOf(constArray) == storeType) {
            LLVMBuildStore(ctx->builder, constArray, destPtr);
            CG_STORE_INIT_RETURN(true);
        }
    }

    LLVMValueRef casted = cg_cast_value(ctx, value, storeType, cg_resolve_expression_type(ctx, expr), destParsed, "init.cast");
    LLVMBuildStore(ctx->builder, casted, destPtr);
    CG_STORE_INIT_RETURN(true);
#undef CG_STORE_INIT_RETURN
    return false;
}

static bool cg_store_struct_entries(CodegenContext* ctx,
                                    LLVMValueRef destPtr,
                                    LLVMTypeRef destType,
                                    const ParsedType* destParsed,
                                    DesignatedInit** entries,
                                    size_t entryCount,
                                    bool zeroInitialized);
static bool cg_store_struct_flat_entries(CodegenContext* ctx,
                                         LLVMValueRef destPtr,
                                         LLVMTypeRef destType,
                                         const ParsedType* destParsed,
                                         DesignatedInit** entries,
                                         size_t entryCount,
                                         size_t* cursor);
static bool cg_store_array_entries(CodegenContext* ctx,
                                   LLVMValueRef destPtr,
                                   LLVMTypeRef destType,
                                   const ParsedType* destParsed,
                                   DesignatedInit** entries,
                                    size_t entryCount);
static bool cg_store_array_flat_entries(CodegenContext* ctx,
                                        LLVMValueRef destPtr,
                                        LLVMTypeRef destType,
                                        const ParsedType* destParsed,
                                        DesignatedInit** entries,
                                        size_t entryCount,
                                        size_t* cursor);
static bool cg_store_designated_entries_impl(CodegenContext* ctx,
                                             LLVMValueRef destPtr,
                                             LLVMTypeRef destType,
                                             const ParsedType* destParsed,
                                             DesignatedInit** entries,
                                             size_t entryCount,
                                             bool zeroInitialize);
static bool cg_store_compound_literal_into_ptr_impl(CodegenContext* ctx,
                                                    LLVMValueRef destPtr,
                                                    LLVMTypeRef destType,
                                                    const ParsedType* destParsed,
                                                    ASTNode* literalNode,
                                                    bool zeroInitialize);

bool cg_store_designated_entries(CodegenContext* ctx,
                                 LLVMValueRef destPtr,
                                 LLVMTypeRef destType,
                                 const ParsedType* destParsed,
                                 DesignatedInit** entries,
                                 size_t entryCount) {
    return cg_store_designated_entries_impl(ctx,
                                            destPtr,
                                            destType,
                                            destParsed,
                                            entries,
                                            entryCount,
                                            true);
}

static bool cg_store_designated_entries_impl(CodegenContext* ctx,
                                             LLVMValueRef destPtr,
                                             LLVMTypeRef destType,
                                             const ParsedType* destParsed,
                                             DesignatedInit** entries,
                                             size_t entryCount,
                                             bool zeroInitialize) {
    if (!ctx || !destPtr || !destType || LLVMGetTypeKind(destType) == LLVMVoidTypeKind) {
        return false;
    }

    LLVMTypeKind kind = LLVMGetTypeKind(destType);
    if (kind == LLVMStructTypeKind || kind == LLVMArrayTypeKind) {
        /*
         * C aggregate initialization is fail-closed: any field/element not explicitly
         * initialized must become zero-initialized. Seed with a full byte-wise zero
         * before applying explicit designators/entries.
         */
        if (zeroInitialize &&
            !cg_zero_initialize_storage(ctx, destPtr, destType, destParsed)) {
            return false;
        }

        if (kind == LLVMStructTypeKind) {
            return cg_store_struct_entries(ctx,
                                           destPtr,
                                           destType,
                                           destParsed,
                                           entries,
                                           entryCount,
                                           zeroInitialize);
        }
        return cg_store_array_entries(ctx, destPtr, destType, destParsed, entries, entryCount);
    }

    if (entryCount != 1 || !entries[0] || !entries[0]->expression) {
        fprintf(stderr, "Error: scalar initializer requires single expression\n");
        return false;
    }
    return cg_store_initializer_expression(ctx, destPtr, destType, destParsed, entries[0]->expression);
}

bool cg_store_compound_literal_into_ptr(CodegenContext* ctx,
                                        LLVMValueRef destPtr,
                                        LLVMTypeRef destType,
                                        const ParsedType* destParsed,
                                        ASTNode* literalNode) {
    return cg_store_compound_literal_into_ptr_impl(ctx,
                                                   destPtr,
                                                   destType,
                                                   destParsed,
                                                   literalNode,
                                                   true);
}

static bool cg_store_compound_literal_into_ptr_impl(CodegenContext* ctx,
                                                    LLVMValueRef destPtr,
                                                    LLVMTypeRef destType,
                                                    const ParsedType* destParsed,
                                                    ASTNode* literalNode,
                                                    bool zeroInitialize) {
    if (!ctx || !destPtr || !literalNode || literalNode->type != AST_COMPOUND_LITERAL) {
        return false;
    }

    const ParsedType* literalParsed = &literalNode->compoundLiteral.literalType;
    if (literalParsed && literalParsed->kind != TYPE_INVALID) {
        destParsed = literalParsed;
        LLVMTypeRef literalLLVM = cg_type_from_parsed(ctx, literalParsed);
        if (literalLLVM && LLVMGetTypeKind(literalLLVM) != LLVMVoidTypeKind) {
            destType = literalLLVM;
        }
    }

    if (!destType || LLVMGetTypeKind(destType) == LLVMVoidTypeKind) {
        fprintf(stderr, "Error: Unable to resolve compound literal type\n");
        return false;
    }

    return cg_store_designated_entries_impl(ctx,
                                            destPtr,
                                            destType,
                                            destParsed,
                                            literalNode->compoundLiteral.entries,
                                            literalNode->compoundLiteral.entryCount,
                                            zeroInitialize);
}

static bool cg_store_struct_entries(CodegenContext* ctx,
                                    LLVMValueRef destPtr,
                                    LLVMTypeRef destType,
                                    const ParsedType* destParsed,
                                    DesignatedInit** entries,
                                    size_t entryCount,
                                    bool zeroInitialized) {
    if (!ctx || !destPtr || LLVMGetTypeKind(destType) != LLVMStructTypeKind) {
        return false;
    }

    const char* structName = destParsed ? destParsed->userTypeName : NULL;
    CGStructLLVMInfo* structInfo =
        cg_init_find_struct_info_for_aggregate(ctx, destType, destParsed);
    if (!structInfo &&
        !structName &&
        destType &&
        LLVMGetTypeKind(destType) == LLVMStructTypeKind) {
        structName = LLVMGetStructName(destType);
    }
    const StructInfo* legacyInfo =
        cg_init_lookup_legacy_struct_info(ctx, structName, destType);
    bool isUnionAggregate =
        (destParsed &&
         (destParsed->tag == TAG_UNION || destParsed->kind == TYPE_UNION)) ||
        (structInfo && structInfo->isUnion) ||
        (legacyInfo && legacyInfo->isUnion);
    const char* activeUnionField = NULL;
    bool seenUnionField = false;

    unsigned implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned targetIndex = implicitIndex;
        const ParsedType* fieldParsed = NULL;
        const char* targetFieldName = entry->fieldName;

        bool matchedField = false;
        if (entry->fieldName && structInfo) {
            for (size_t f = 0; f < structInfo->fieldCount; ++f) {
                if (structInfo->fields[f].name &&
                    strcmp(structInfo->fields[f].name, entry->fieldName) == 0) {
                    targetIndex = structInfo->fields[f].index;
                    fieldParsed = &structInfo->fields[f].parsedType;
                    matchedField = true;
                    break;
                }
            }
        }
        if (entry->fieldName && !matchedField && legacyInfo) {
            for (size_t f = 0; f < legacyInfo->fieldCount; ++f) {
                if (legacyInfo->fields[f].name &&
                    strcmp(legacyInfo->fields[f].name, entry->fieldName) == 0) {
                    targetIndex = legacyInfo->fields[f].index;
                    fieldParsed = &legacyInfo->fields[f].parsedType;
                    matchedField = true;
                    break;
                }
            }
        }
        if (!entry->fieldName && structInfo && targetIndex < structInfo->fieldCount) {
            targetFieldName = structInfo->fields[targetIndex].name;
            fieldParsed = &structInfo->fields[targetIndex].parsedType;
        }
        if (!entry->fieldName &&
            !targetFieldName &&
            legacyInfo &&
            targetIndex < legacyInfo->fieldCount) {
            targetFieldName = legacyInfo->fields[targetIndex].name;
            fieldParsed = &legacyInfo->fields[targetIndex].parsedType;
        }
        if (!targetFieldName) {
            (void)cg_init_field_by_index(ctx, destParsed, targetIndex, &targetFieldName, &fieldParsed);
        }
        (void)matchedField;
        implicitIndex = targetIndex + 1;

        if (isUnionAggregate && targetFieldName) {
            bool shouldResetUnion = false;
            if (!seenUnionField) {
                shouldResetUnion = !zeroInitialized;
            } else if (strcmp(activeUnionField, targetFieldName) != 0) {
                shouldResetUnion = true;
            }
            if (shouldResetUnion &&
                !cg_zero_initialize_storage(ctx, destPtr, destType, destParsed)) {
                return false;
            }
            activeUnionField = targetFieldName;
            seenUnionField = true;
            zeroInitialized = true;
        }

        if (targetIndex >= LLVMCountStructElementTypes(destType) && !targetFieldName) {
            continue;
        }

        const CCTagFieldLayout* lay =
            cg_init_lookup_field_layout(ctx, destType, destParsed, targetFieldName);
        ASTNode* valueExpr = entry->expression;
        const ParsedType* valueParsed = cg_resolve_expression_type(ctx, valueExpr);

        if (lay && lay->isBitfield && lay->widthBits > 0) {
            LLVMValueRef value = codegenNode(ctx, valueExpr);
            if (!value) {
                return false;
            }
            if (!cg_init_store_bitfield(ctx, destPtr, lay, value, valueParsed, fieldParsed)) {
                return false;
            }
            continue;
        }

        LLVMTypeRef fieldType = NULL;
        LLVMValueRef fieldPtr = NULL;
        if (targetFieldName) {
            fieldPtr = buildStructFieldPointer(ctx,
                                               destPtr,
                                               destType,
                                               structName,
                                               targetFieldName,
                                               destParsed,
                                               &fieldType,
                                               &fieldParsed);
        }
        if (!fieldPtr) {
            if (targetIndex >= LLVMCountStructElementTypes(destType)) {
                continue;
            }
            fieldPtr = LLVMBuildStructGEP2(ctx->builder, destType, destPtr, targetIndex, "init.field");
            if (!fieldPtr) {
                fprintf(stderr, "Error: Unable to access struct field for initializer\n");
                return false;
            }
            fieldType = LLVMStructGetTypeAtIndex(destType, targetIndex);
        }

        size_t mergedLast = i;
        DesignatedInit** mergedEntries = NULL;
        size_t mergedCount = 0;
        if (targetFieldName &&
            !entry->resetSubobjectBeforeStore &&
            valueExpr->type == AST_COMPOUND_LITERAL &&
            cg_entries_have_designators(valueExpr->compoundLiteral.entries,
                                        valueExpr->compoundLiteral.entryCount) &&
            (LLVMGetTypeKind(fieldType) == LLVMArrayTypeKind ||
             LLVMGetTypeKind(fieldType) == LLVMStructTypeKind)) {
            mergedCount = valueExpr->compoundLiteral.entryCount;
            for (size_t j = i + 1; j < entryCount; ++j) {
                DesignatedInit* next = entries[j];
                if (!next || !next->expression || !next->fieldName) {
                    break;
                }
                if (strcmp(next->fieldName, targetFieldName) != 0 ||
                    next->resetSubobjectBeforeStore ||
                    next->expression->type != AST_COMPOUND_LITERAL ||
                    !cg_entries_have_designators(next->expression->compoundLiteral.entries,
                                                 next->expression->compoundLiteral.entryCount)) {
                    break;
                }
                mergedCount += next->expression->compoundLiteral.entryCount;
                mergedLast = j;
            }
            if (mergedLast > i) {
                mergedEntries = (DesignatedInit**)calloc(mergedCount, sizeof(DesignatedInit*));
                if (!mergedEntries) {
                    return false;
                }
                size_t cursor = 0;
                for (size_t j = i; j <= mergedLast; ++j) {
                    DesignatedInit* next = entries[j];
                    for (size_t k = 0; k < next->expression->compoundLiteral.entryCount; ++k) {
                        mergedEntries[cursor++] = next->expression->compoundLiteral.entries[k];
                    }
                }
            }
        }

        if (mergedEntries) {
            bool ok = cg_store_designated_entries_impl(ctx,
                                                       fieldPtr,
                                                       fieldType,
                                                       fieldParsed,
                                                       mergedEntries,
                                                       mergedCount,
                                                       false);
            free(mergedEntries);
            if (!ok) {
                return false;
            }
            i = mergedLast;
            continue;
        }

        if (valueExpr->type == AST_COMPOUND_LITERAL) {
            bool zeroInitialize =
                entry->resetSubobjectBeforeStore ||
                !cg_entries_have_designators(valueExpr->compoundLiteral.entries,
                                             valueExpr->compoundLiteral.entryCount);
            if (!cg_store_compound_literal_into_ptr_impl(ctx,
                                                         fieldPtr,
                                                         fieldType,
                                                         fieldParsed,
                                                         valueExpr,
                                                         zeroInitialize)) {
                return false;
            }
        } else {
            if (!cg_store_initializer_expression(ctx, fieldPtr, fieldType, fieldParsed, valueExpr)) {
                return false;
            }
        }
    }
    return true;
}

static bool cg_store_struct_flat_entries(CodegenContext* ctx,
                                         LLVMValueRef destPtr,
                                         LLVMTypeRef destType,
                                         const ParsedType* destParsed,
                                         DesignatedInit** entries,
                                         size_t entryCount,
                                         size_t* cursor) {
    if (!ctx || !destPtr || !cursor || LLVMGetTypeKind(destType) != LLVMStructTypeKind) {
        return false;
    }

    unsigned fieldCount = LLVMCountStructElementTypes(destType);
    for (unsigned fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
        if (*cursor >= entryCount) {
            break;
        }

        const char* fieldName = NULL;
        const ParsedType* fieldParsed = NULL;
        (void)cg_init_field_by_index(ctx, destParsed, fieldIndex, &fieldName, &fieldParsed);

        LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(destType, fieldIndex);
        LLVMValueRef fieldPtr =
            LLVMBuildStructGEP2(ctx->builder, destType, destPtr, fieldIndex, "init.flat.field");
        if (!fieldPtr || !fieldType) {
            return false;
        }

        DesignatedInit* entry = entries[*cursor];
        if (!entry || !entry->expression) {
            (*cursor)++;
            continue;
        }

        if (entry->expression->type == AST_COMPOUND_LITERAL) {
            if (!cg_store_compound_literal_into_ptr(ctx,
                                                    fieldPtr,
                                                    fieldType,
                                                    fieldParsed,
                                                    entry->expression)) {
                return false;
            }
            (*cursor)++;
            continue;
        }

        if (cg_expr_initializes_whole_aggregate(ctx, entry->expression, fieldType)) {
            if (!cg_store_initializer_expression(ctx, fieldPtr, fieldType, fieldParsed, entry->expression)) {
                return false;
            }
            (*cursor)++;
            continue;
        }

        if (LLVMGetTypeKind(fieldType) == LLVMArrayTypeKind) {
            if (!cg_store_array_flat_entries(ctx,
                                             fieldPtr,
                                             fieldType,
                                             fieldParsed,
                                             entries,
                                             entryCount,
                                             cursor)) {
                return false;
            }
            continue;
        }

        if (LLVMGetTypeKind(fieldType) == LLVMStructTypeKind) {
            if (!cg_store_struct_flat_entries(ctx,
                                              fieldPtr,
                                              fieldType,
                                              fieldParsed,
                                              entries,
                                              entryCount,
                                              cursor)) {
                return false;
            }
            continue;
        }

        if (!cg_store_initializer_expression(ctx, fieldPtr, fieldType, fieldParsed, entry->expression)) {
            return false;
        }
        (*cursor)++;
    }

    return true;
}

static bool cg_store_array_flat_entries(CodegenContext* ctx,
                                        LLVMValueRef destPtr,
                                        LLVMTypeRef destType,
                                        const ParsedType* destParsed,
                                        DesignatedInit** entries,
                                        size_t entryCount,
                                        size_t* cursor) {
    if (!ctx || !destPtr || !cursor || LLVMGetTypeKind(destType) != LLVMArrayTypeKind) {
        return false;
    }

    LLVMTypeRef elementType = LLVMGetElementType(destType);
    ParsedType elementParsedStorage = {0};
    const ParsedType* elementParsed = NULL;
    bool hasElementParsed = false;
    if (destParsed && parsedTypeIsDirectArray(destParsed)) {
        elementParsedStorage = parsedTypeArrayElementType(destParsed);
        elementParsed = &elementParsedStorage;
        hasElementParsed = true;
    }

    unsigned length = LLVMGetArrayLength(destType);
    LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
    for (unsigned i = 0; i < length; ++i) {
        if (*cursor >= entryCount) {
            break;
        }

        DesignatedInit* entry = entries[*cursor];
        if (!entry || !entry->expression) {
            (*cursor)++;
            continue;
        }

        LLVMValueRef idxVals[2] = {
            zero,
            LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), i, 0)
        };
        LLVMValueRef elementPtr =
            LLVMBuildGEP2(ctx->builder, destType, destPtr, idxVals, 2, "init.flat.elem");

        if (entry->expression->type == AST_COMPOUND_LITERAL) {
            bool zeroInitialize =
                entry->resetSubobjectBeforeStore ||
                !cg_entries_have_designators(entry->expression->compoundLiteral.entries,
                                             entry->expression->compoundLiteral.entryCount);
            if (!cg_store_compound_literal_into_ptr_impl(ctx,
                                                         elementPtr,
                                                         elementType,
                                                         elementParsed,
                                                         entry->expression,
                                                         zeroInitialize)) {
                if (hasElementParsed) parsedTypeFree(&elementParsedStorage);
                return false;
            }
            (*cursor)++;
            continue;
        }

        if (cg_expr_initializes_whole_aggregate(ctx, entry->expression, elementType)) {
            if (!cg_store_initializer_expression(ctx, elementPtr, elementType, elementParsed, entry->expression)) {
                if (hasElementParsed) parsedTypeFree(&elementParsedStorage);
                return false;
            }
            (*cursor)++;
            continue;
        }

        if (LLVMGetTypeKind(elementType) == LLVMArrayTypeKind) {
            if (!cg_store_array_flat_entries(ctx,
                                             elementPtr,
                                             elementType,
                                             elementParsed,
                                             entries,
                                             entryCount,
                                             cursor)) {
                if (hasElementParsed) parsedTypeFree(&elementParsedStorage);
                return false;
            }
            continue;
        }

        if (LLVMGetTypeKind(elementType) == LLVMStructTypeKind) {
            if (!cg_store_struct_flat_entries(ctx,
                                              elementPtr,
                                              elementType,
                                              elementParsed,
                                              entries,
                                              entryCount,
                                              cursor)) {
                if (hasElementParsed) parsedTypeFree(&elementParsedStorage);
                return false;
            }
            continue;
        }

        if (!cg_store_initializer_expression(ctx, elementPtr, elementType, elementParsed, entry->expression)) {
            if (hasElementParsed) parsedTypeFree(&elementParsedStorage);
            return false;
        }
        (*cursor)++;
    }

    if (hasElementParsed) {
        parsedTypeFree(&elementParsedStorage);
    }
    return true;
}

static bool cg_store_array_entries(CodegenContext* ctx,
                                   LLVMValueRef destPtr,
                                   LLVMTypeRef destType,
                                   const ParsedType* destParsed,
                                   DesignatedInit** entries,
                                   size_t entryCount) {
    if (!ctx || !destPtr || LLVMGetTypeKind(destType) != LLVMArrayTypeKind) {
        return false;
    }

    LLVMTypeRef elementType = LLVMGetElementType(destType);
    if ((LLVMGetTypeKind(elementType) == LLVMArrayTypeKind ||
         LLVMGetTypeKind(elementType) == LLVMStructTypeKind) &&
        cg_entries_flat_scalars(entries, entryCount)) {
        bool hasWholeAggregateEntries = false;
        for (size_t i = 0; i < entryCount; ++i) {
            DesignatedInit* entry = entries[i];
            if (!entry || !entry->expression) continue;
            if (cg_expr_initializes_whole_aggregate(ctx, entry->expression, elementType)) {
                hasWholeAggregateEntries = true;
                break;
            }
        }
        if (!hasWholeAggregateEntries) {
        size_t cursor = 0;
        return cg_store_array_flat_entries(ctx,
                                           destPtr,
                                           destType,
                                           destParsed,
                                           entries,
                                           entryCount,
                                           &cursor);
        }
    }

    unsigned long long implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned long long targetIndex = implicitIndex;
        if (entry->indexExpr) {
            bool ok = false;
            targetIndex = cg_eval_initializer_index(entry->indexExpr, &ok);
            if (!ok) return false;
        }
        implicitIndex = targetIndex + 1;

        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef idxVals[2] = {
            zero,
            LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), (unsigned)targetIndex, 0)
        };
        LLVMValueRef elementPtr = LLVMBuildGEP2(ctx->builder, destType, destPtr, idxVals, 2, "init.elem");
        ParsedType elementParsedStorage = {0};
        const ParsedType* elementParsed = NULL;
        bool hasElementParsed = false;
        if (destParsed && parsedTypeIsDirectArray(destParsed)) {
            elementParsedStorage = parsedTypeArrayElementType(destParsed);
            elementParsed = &elementParsedStorage;
            hasElementParsed = true;
        }

        size_t mergedLast = i;
        DesignatedInit** mergedEntries = NULL;
        size_t mergedCount = 0;
        if (!entry->resetSubobjectBeforeStore &&
            entry->expression->type == AST_COMPOUND_LITERAL &&
            cg_entries_have_designators(entry->expression->compoundLiteral.entries,
                                        entry->expression->compoundLiteral.entryCount) &&
            (LLVMGetTypeKind(elementType) == LLVMArrayTypeKind ||
             LLVMGetTypeKind(elementType) == LLVMStructTypeKind)) {
            mergedCount = entry->expression->compoundLiteral.entryCount;
            unsigned long long scanImplicitIndex = implicitIndex;
            for (size_t j = i + 1; j < entryCount; ++j) {
                DesignatedInit* next = entries[j];
                if (!next || !next->expression) {
                    break;
                }
                unsigned long long nextIndex = scanImplicitIndex;
                if (next->indexExpr) {
                    bool ok = false;
                    nextIndex = cg_eval_initializer_index(next->indexExpr, &ok);
                    if (!ok) {
                        if (hasElementParsed) {
                            parsedTypeFree(&elementParsedStorage);
                        }
                        return false;
                    }
                }
                scanImplicitIndex = nextIndex + 1;
                if (nextIndex != targetIndex ||
                    next->resetSubobjectBeforeStore ||
                    next->expression->type != AST_COMPOUND_LITERAL ||
                    !cg_entries_have_designators(next->expression->compoundLiteral.entries,
                                                 next->expression->compoundLiteral.entryCount)) {
                    break;
                }
                mergedCount += next->expression->compoundLiteral.entryCount;
                mergedLast = j;
            }
            if (mergedLast > i) {
                mergedEntries = (DesignatedInit**)calloc(mergedCount, sizeof(DesignatedInit*));
                if (!mergedEntries) {
                    if (hasElementParsed) {
                        parsedTypeFree(&elementParsedStorage);
                    }
                    return false;
                }
                size_t cursor = 0;
                for (size_t j = i; j <= mergedLast; ++j) {
                    DesignatedInit* next = entries[j];
                    for (size_t k = 0; k < next->expression->compoundLiteral.entryCount; ++k) {
                        mergedEntries[cursor++] = next->expression->compoundLiteral.entries[k];
                    }
                }
                implicitIndex = scanImplicitIndex;
            }
        }

        if (mergedEntries) {
            bool ok = cg_store_designated_entries_impl(ctx,
                                                       elementPtr,
                                                       elementType,
                                                       elementParsed,
                                                       mergedEntries,
                                                       mergedCount,
                                                       false);
            free(mergedEntries);
            if (hasElementParsed) {
                parsedTypeFree(&elementParsedStorage);
            }
            if (!ok) {
                return false;
            }
            i = mergedLast;
            continue;
        }

        if (entry->expression->type == AST_COMPOUND_LITERAL) {
            bool zeroInitialize =
                entry->resetSubobjectBeforeStore ||
                !cg_entries_have_designators(entry->expression->compoundLiteral.entries,
                                             entry->expression->compoundLiteral.entryCount);
            if (!cg_store_compound_literal_into_ptr_impl(ctx,
                                                         elementPtr,
                                                         elementType,
                                                         elementParsed,
                                                         entry->expression,
                                                         zeroInitialize)) {
                if (hasElementParsed) parsedTypeFree(&elementParsedStorage);
                return false;
            }
        } else {
            if (!cg_store_initializer_expression(ctx, elementPtr, elementType, elementParsed, entry->expression)) {
                if (hasElementParsed) parsedTypeFree(&elementParsedStorage);
                return false;
            }
        }
        if (hasElementParsed) {
            parsedTypeFree(&elementParsedStorage);
        }
    }
    return true;
}
