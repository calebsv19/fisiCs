#include "codegen_private.h"

#include "codegen_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long cg_eval_initializer_index(ASTNode* expr, bool* outSuccess) {
    if (outSuccess) *outSuccess = false;
    if (!expr) return 0;
    unsigned long long result = 0;
    switch (expr->type) {
        case AST_NUMBER_LITERAL:
            result = strtoull(expr->valueNode.value, NULL, 0);
            if (outSuccess) *outSuccess = true;
            return result;
        case AST_CHAR_LITERAL:
            if (expr->valueNode.value && expr->valueNode.value[0] != '\0') {
                result = (unsigned char)expr->valueNode.value[0];
                if (outSuccess) *outSuccess = true;
                return result;
            }
            break;
        default:
            break;
    }
    fprintf(stderr, "Error: Initializer designator index must be an integer constant\n");
    return 0;
}

bool cg_store_initializer_expression(CodegenContext* ctx,
                                     LLVMValueRef destPtr,
                                     LLVMTypeRef destType,
                                     const ParsedType* destParsed,
                                     ASTNode* expr) {
    if (!ctx || !destPtr || !expr) return false;
    LLVMValueRef value = codegenNode(ctx, expr);
    if (!value) {
        fprintf(stderr, "Error: Failed to evaluate initializer expression\n");
        return false;
    }

    LLVMTypeRef storeType = destType;
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        storeType = LLVMTypeOf(value);
    }
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        fprintf(stderr, "Error: Invalid initializer destination type\n");
        return false;
    }

    LLVMValueRef casted = cg_cast_value(ctx, value, storeType, cg_resolve_expression_type(ctx, expr), destParsed, "init.cast");
    LLVMBuildStore(ctx->builder, casted, destPtr);
    return true;
}

static bool cg_store_struct_entries(CodegenContext* ctx,
                                    LLVMValueRef destPtr,
                                    LLVMTypeRef destType,
                                    const ParsedType* destParsed,
                                    DesignatedInit** entries,
                                    size_t entryCount);
static bool cg_store_array_entries(CodegenContext* ctx,
                                   LLVMValueRef destPtr,
                                   LLVMTypeRef destType,
                                   const ParsedType* destParsed,
                                   DesignatedInit** entries,
                                   size_t entryCount);

bool cg_store_designated_entries(CodegenContext* ctx,
                                 LLVMValueRef destPtr,
                                 LLVMTypeRef destType,
                                 const ParsedType* destParsed,
                                 DesignatedInit** entries,
                                 size_t entryCount) {
    if (!ctx || !destPtr || !destType || LLVMGetTypeKind(destType) == LLVMVoidTypeKind) {
        return false;
    }

    LLVMTypeKind kind = LLVMGetTypeKind(destType);
    if (kind == LLVMStructTypeKind || kind == LLVMArrayTypeKind) {
        if (kind == LLVMStructTypeKind) {
            return cg_store_struct_entries(ctx, destPtr, destType, destParsed, entries, entryCount);
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

    return cg_store_designated_entries(ctx,
                                       destPtr,
                                       destType,
                                       destParsed,
                                       literalNode->compoundLiteral.entries,
                                       literalNode->compoundLiteral.entryCount);
}

static bool cg_store_struct_entries(CodegenContext* ctx,
                                    LLVMValueRef destPtr,
                                    LLVMTypeRef destType,
                                    const ParsedType* destParsed,
                                    DesignatedInit** entries,
                                    size_t entryCount) {
    if (!ctx || !destPtr || LLVMGetTypeKind(destType) != LLVMStructTypeKind) {
        return false;
    }

    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    const char* structName = destParsed ? destParsed->userTypeName : NULL;
    CGStructLLVMInfo* structInfo = NULL;
    if (cache && structName) {
        structInfo = cg_type_cache_get_struct_info(cache, structName);
    }

    unsigned implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned targetIndex = implicitIndex;
        const ParsedType* fieldParsed = NULL;

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
        if (!matchedField) {
            implicitIndex = targetIndex + 1;
        }

        LLVMValueRef fieldPtr = LLVMBuildStructGEP2(ctx->builder, destType, destPtr, targetIndex, "init.field");
        if (!fieldPtr) {
            fprintf(stderr, "Error: Unable to access struct field for initializer\n");
            return false;
        }
        LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(destType, targetIndex);
        ASTNode* valueExpr = entry->expression;
        if (valueExpr->type == AST_COMPOUND_LITERAL) {
            if (!cg_store_compound_literal_into_ptr(ctx, fieldPtr, fieldType, fieldParsed, valueExpr)) {
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

static bool cg_store_array_entries(CodegenContext* ctx,
                                   LLVMValueRef destPtr,
                                   LLVMTypeRef destType,
                                   const ParsedType* destParsed,
                                   DesignatedInit** entries,
                                   size_t entryCount) {
    if (!ctx || !destPtr || LLVMGetTypeKind(destType) != LLVMArrayTypeKind) {
        return false;
    }
    (void)destParsed;

    LLVMTypeRef elementType = LLVMGetElementType(destType);
    unsigned long long implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned long long targetIndex = implicitIndex;
        if (entry->indexExpr) {
            bool ok = false;
            targetIndex = cg_eval_initializer_index(entry->indexExpr, &ok);
            if (!ok) return false;
        } else {
            implicitIndex = targetIndex + 1;
        }

        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef idxVals[2] = {
            zero,
            LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), (unsigned)targetIndex, 0)
        };
        LLVMValueRef elementPtr = LLVMBuildGEP2(ctx->builder, destType, destPtr, idxVals, 2, "init.elem");
        const ParsedType* elementParsed = NULL;
        if (entry->expression->type == AST_COMPOUND_LITERAL) {
            if (!cg_store_compound_literal_into_ptr(ctx, elementPtr, elementType, elementParsed, entry->expression)) {
                return false;
            }
        } else {
            if (!cg_store_initializer_expression(ctx, elementPtr, elementType, elementParsed, entry->expression)) {
                return false;
            }
        }
    }
    return true;
}
