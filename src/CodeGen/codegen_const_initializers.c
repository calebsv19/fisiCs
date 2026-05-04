// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include "Syntax/const_eval.h"
#include "codegen_const_initializers_internal.h"
#include "codegen_types.h"
#include "literal_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_const_string_counter = 0;

static bool cg_named_type_has_surface_derivations(const ParsedType* type) {
    if (!type || type->kind != TYPE_NAMED) {
        return false;
    }
    return type->derivationCount > 0 || type->pointerDepth > 0 || type->isFunctionPointer;
}

const ParsedType* cg_resolve_typedef_parsed(CodegenContext* ctx, const ParsedType* type) {
    if (!ctx || !type || type->kind != TYPE_NAMED || !type->userTypeName) {
        return type;
    }
    if (cg_named_type_has_surface_derivations(type)) {
        return type;
    }
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (!cache) return type;
    CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, type->userTypeName);
    if (info) {
        return &info->parsedType;
    }
    if (ctx->semanticModel) {
        const Symbol* sym = semanticModelLookupGlobal(ctx->semanticModel, type->userTypeName);
        if (sym && sym->kind == SYMBOL_TYPEDEF) {
            return &sym->type;
        }
    }
    return type;
}

static bool cg_eval_const_float_expr(ASTNode* expr, double* outValue) {
    if (!expr || !outValue) return false;

    switch (expr->type) {
        case AST_NUMBER_LITERAL:
        case AST_CHAR_LITERAL: {
            const char* text = expr->valueNode.value ? expr->valueNode.value : "0";
            char* end = NULL;
            double value = strtod(text, &end);
            if (end == text) return false;
            *outValue = value;
            return true;
        }
        case AST_CAST_EXPRESSION:
            return cg_eval_const_float_expr(expr->castExpr.expression, outValue);
        case AST_UNARY_EXPRESSION: {
            if (!expr->expr.op || !expr->expr.left) return false;
            double inner = 0.0;
            if (!cg_eval_const_float_expr(expr->expr.left, &inner)) return false;
            if (strcmp(expr->expr.op, "+") == 0) {
                *outValue = inner;
                return true;
            }
            if (strcmp(expr->expr.op, "-") == 0) {
                *outValue = -inner;
                return true;
            }
            return false;
        }
        case AST_BINARY_EXPRESSION: {
            if (!expr->expr.op || !expr->expr.left || !expr->expr.right) return false;
            double lhs = 0.0;
            double rhs = 0.0;
            if (!cg_eval_const_float_expr(expr->expr.left, &lhs) ||
                !cg_eval_const_float_expr(expr->expr.right, &rhs)) {
                return false;
            }
            if (strcmp(expr->expr.op, "+") == 0) {
                *outValue = lhs + rhs;
                return true;
            }
            if (strcmp(expr->expr.op, "-") == 0) {
                *outValue = lhs - rhs;
                return true;
            }
            if (strcmp(expr->expr.op, "*") == 0) {
                *outValue = lhs * rhs;
                return true;
            }
            if (strcmp(expr->expr.op, "/") == 0) {
                if (rhs == 0.0) return false;
                *outValue = lhs / rhs;
                return true;
            }
            return false;
        }
        case AST_TERNARY_EXPRESSION: {
            if (!expr->ternaryExpr.condition ||
                !expr->ternaryExpr.trueExpr ||
                !expr->ternaryExpr.falseExpr) {
                return false;
            }
            double cond = 0.0;
            if (!cg_eval_const_float_expr(expr->ternaryExpr.condition, &cond)) return false;
            ASTNode* chosen = (cond != 0.0) ? expr->ternaryExpr.trueExpr : expr->ternaryExpr.falseExpr;
            return cg_eval_const_float_expr(chosen, outValue);
        }
        case AST_COMMA_EXPRESSION: {
            if (!expr->commaExpr.expressions || expr->commaExpr.exprCount == 0) return false;
            double last = 0.0;
            for (size_t i = 0; i < expr->commaExpr.exprCount; ++i) {
                if (!cg_eval_const_float_expr(expr->commaExpr.expressions[i], &last)) {
                    return false;
                }
            }
            *outValue = last;
            return true;
        }
        default:
            return false;
    }
}

static LLVMValueRef cg_make_const_string_global(CodegenContext* ctx,
                                                const char* contents,
                                                LLVMTypeRef targetPtrType) {
    if (!ctx || !contents || !targetPtrType) return NULL;
    size_t len = strlen(contents);
    LLVMTypeRef i8 = LLVMInt8TypeInContext(ctx->llvmContext);
    LLVMTypeRef arrayTy = LLVMArrayType(i8, (unsigned)(len + 1));

    char name[64];
    snprintf(name, sizeof(name), ".str.%d", g_const_string_counter++);

    LLVMValueRef global = LLVMAddGlobal(ctx->module, arrayTy, name);
    LLVMSetLinkage(global, LLVMPrivateLinkage);
    LLVMSetGlobalConstant(global, 1);
    LLVMSetUnnamedAddr(global, LLVMGlobalUnnamedAddr);
    LLVMValueRef strConst = LLVMConstStringInContext(ctx->llvmContext, contents, (unsigned)len, 0);
    LLVMSetInitializer(global, strConst);

    LLVMTypeRef defaultPtr = LLVMPointerType(i8, 0);
    LLVMValueRef basePtr = LLVMConstBitCast(global, defaultPtr);
    if (LLVMTypeOf(basePtr) == targetPtrType) {
        return basePtr;
    }
    return LLVMConstBitCast(basePtr, targetPtrType);
}

static LLVMValueRef cg_make_const_wstring_global(CodegenContext* ctx,
                                                 const char* payload,
                                                 LLVMTypeRef targetPtrType) {
    if (!ctx || !targetPtrType) return NULL;
    uint32_t* codepoints = NULL;
    size_t count = 0;
    LiteralDecodeResult res = decode_c_string_literal_wide(payload ? payload : "",
                                                           32,
                                                           &codepoints,
                                                           &count);
    (void)res;

    LLVMTypeRef elemTy = LLVMInt32TypeInContext(ctx->llvmContext);
    LLVMTypeRef arrayTy = LLVMArrayType(elemTy, (unsigned)(count + 1));
    LLVMValueRef* values = (LLVMValueRef*)calloc(count + 1, sizeof(LLVMValueRef));
    if (!values) {
        free(codepoints);
        return NULL;
    }
    for (size_t i = 0; i < count; ++i) {
        values[i] = LLVMConstInt(elemTy, codepoints ? codepoints[i] : 0, 0);
    }
    values[count] = LLVMConstInt(elemTy, 0, 0);

    char name[64];
    snprintf(name, sizeof(name), ".wstr.%d", g_const_string_counter++);
    LLVMValueRef global = LLVMAddGlobal(ctx->module, arrayTy, name);
    LLVMSetLinkage(global, LLVMPrivateLinkage);
    LLVMSetGlobalConstant(global, 1);
    LLVMSetUnnamedAddr(global, LLVMGlobalUnnamedAddr);
    LLVMSetInitializer(global, LLVMConstArray(elemTy, values, (unsigned)(count + 1)));

    LLVMTypeRef defaultPtr = LLVMPointerType(elemTy, 0);
    LLVMValueRef basePtr = LLVMConstBitCast(global, defaultPtr);
    free(values);
    free(codepoints);
    if (LLVMTypeOf(basePtr) == targetPtrType) {
        return basePtr;
    }
    return LLVMConstBitCast(basePtr, targetPtrType);
}

static LLVMValueRef cg_const_from_eval(CodegenContext* ctx,
                                       const ConstEvalResult* res,
                                       LLVMTypeRef targetType,
                                       const ParsedType* parsedType) {
    if (!ctx || !res || !res->isConst || !targetType) return NULL;

    LLVMTypeKind kind = LLVMGetTypeKind(targetType);
    if (kind == LLVMIntegerTypeKind) {
        int isUnsigned = parsedType && parsedType->isUnsigned;
        return LLVMConstInt(targetType, (unsigned long long)res->value, isUnsigned ? 0 : 1);
    }
    if (kind == LLVMPointerTypeKind) {
        LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
        int isUnsigned = parsedType && parsedType->isUnsigned;
        LLVMValueRef asInt = LLVMConstInt(intptrTy, (unsigned long long)res->value, isUnsigned ? 0 : 1);
        return LLVMConstIntToPtr(asInt, targetType);
    }
    return NULL;
}

static LLVMValueRef cg_zero_const(LLVMTypeRef type) {
    return type ? LLVMConstNull(type) : NULL;
}

static CGStructLLVMInfo* cg_find_struct_info(CodegenContext* ctx,
                                             LLVMTypeRef llvmType,
                                             const ParsedType* parsedType) {
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (!cache) return NULL;
    if (parsedType && parsedType->userTypeName) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(cache, parsedType->userTypeName);
        if (info) return info;
    }
    if (llvmType) {
        return cg_type_cache_find_struct_by_llvm(cache, llvmType);
    }
    return NULL;
}

unsigned long long cg_eval_initializer_index_const(CodegenContext* ctx,
                                                   ASTNode* expr,
                                                   bool* outSuccess) {
    if (outSuccess) *outSuccess = false;
    if (!ctx || !expr) return 0;
    struct Scope* globalScope = NULL;
    if (ctx->semanticModel) {
        globalScope = semanticModelGetGlobalScope(ctx->semanticModel);
    }
    ConstEvalResult res = constEval(expr, globalScope, true);
    if (res.isConst) {
        if (outSuccess) *outSuccess = true;
        return (unsigned long long)res.value;
    }
    return 0;
}

static bool cg_eval_initializer_signed_const(CodegenContext* ctx,
                                             ASTNode* expr,
                                             long long* outValue) {
    if (!ctx || !expr || !outValue) return false;
    struct Scope* globalScope = NULL;
    if (ctx->semanticModel) {
        globalScope = semanticModelGetGlobalScope(ctx->semanticModel);
    }
    ConstEvalResult res = constEval(expr, globalScope, true);
    if (!res.isConst) return false;
    *outValue = res.value;
    return true;
}

static LLVMValueRef cg_const_resolve_global_lvalue(CodegenContext* ctx,
                                                   ASTNode* expr,
                                                   LLVMTypeRef* outValueType) {
    if (outValueType) *outValueType = NULL;
    if (!ctx || !expr) return NULL;

    if (expr->type == AST_IDENTIFIER) {
        const char* name = expr->valueNode.value;
        if (!name) return NULL;

        LLVMValueRef glob = LLVMGetNamedGlobal(ctx->module, name);
        if (!glob && ctx->semanticModel) {
            const Symbol* sym = semanticModelLookupGlobal(ctx->semanticModel, name);
            if (sym && sym->kind == SYMBOL_VARIABLE) {
                declareGlobalVariableSymbol(ctx, sym);
                glob = LLVMGetNamedGlobal(ctx->module, name);
            }
        }
        if (!glob) return NULL;
        if (outValueType) *outValueType = LLVMGlobalGetValueType(glob);
        return glob;
    }

    if (expr->type == AST_ARRAY_ACCESS &&
        expr->arrayAccess.array &&
        expr->arrayAccess.index) {
        bool indexOk = false;
        unsigned long long index =
            cg_eval_initializer_index_const(ctx, expr->arrayAccess.index, &indexOk);
        if (!indexOk) return NULL;

        LLVMTypeRef baseValueType = NULL;
        LLVMValueRef basePointer =
            cg_const_resolve_global_lvalue(ctx, expr->arrayAccess.array, &baseValueType);
        if (!basePointer || !baseValueType) return NULL;
        if (LLVMGetTypeKind(baseValueType) != LLVMArrayTypeKind) return NULL;

        LLVMTypeRef indexType = cg_get_intptr_type(ctx);
        LLVMValueRef indices[2] = {
            LLVMConstInt(indexType, 0, 0),
            LLVMConstInt(indexType, index, 0),
        };
        LLVMValueRef elementPointer = LLVMConstGEP2(baseValueType, basePointer, indices, 2);
        if (!elementPointer) return NULL;

        if (outValueType) *outValueType = LLVMGetElementType(baseValueType);
        return elementPointer;
    }

    return NULL;
}

static LLVMTypeRef cg_const_pointer_element_type(CodegenContext* ctx,
                                                 const ParsedType* parsedType,
                                                 LLVMTypeRef targetType,
                                                 LLVMValueRef basePointer) {
    if (!ctx) return NULL;

    const ParsedType* resolved = cg_resolve_typedef_parsed(ctx, parsedType);
    if (resolved &&
        (resolved->pointerDepth > 0 ||
         resolved->isFunctionPointer ||
         parsedTypeCountDerivationsOfKind(resolved, TYPE_DERIVATION_POINTER) > 0)) {
        ParsedType elemParsed = parsedTypePointerTargetType(resolved);
        LLVMTypeRef elemType = cg_type_from_parsed(ctx, &elemParsed);
        parsedTypeFree(&elemParsed);
        if (elemType && LLVMGetTypeKind(elemType) != LLVMVoidTypeKind) {
            return elemType;
        }
    }

    if (targetType && LLVMGetTypeKind(targetType) == LLVMPointerTypeKind) {
        LLVMTypeRef targetElem = LLVMGetElementType(targetType);
        if (targetElem && LLVMGetTypeKind(targetElem) != LLVMVoidTypeKind) {
            return targetElem;
        }
    }

    if (basePointer) {
        LLVMTypeRef basePtrType = LLVMTypeOf(basePointer);
        if (basePtrType && LLVMGetTypeKind(basePtrType) == LLVMPointerTypeKind) {
            LLVMTypeRef baseElem = LLVMGetElementType(basePtrType);
            if (baseElem && LLVMGetTypeKind(baseElem) != LLVMVoidTypeKind) {
                return baseElem;
            }
        }
    }
    return NULL;
}

static LLVMValueRef cg_const_pointer_with_element_offset(CodegenContext* ctx,
                                                         LLVMValueRef basePointer,
                                                         long long elementOffset,
                                                         LLVMTypeRef targetType,
                                                         const ParsedType* parsedType) {
    if (!ctx || !basePointer || !targetType) return NULL;
    if (LLVMGetTypeKind(targetType) != LLVMPointerTypeKind) return NULL;

    LLVMTypeRef basePtrType = LLVMTypeOf(basePointer);
    if (!basePtrType || LLVMGetTypeKind(basePtrType) != LLVMPointerTypeKind) {
        return NULL;
    }

    LLVMValueRef result = basePointer;
    if (elementOffset != 0) {
        LLVMTypeRef elementType = cg_const_pointer_element_type(ctx, parsedType, targetType, basePointer);
        if (!elementType) return NULL;
        LLVMTypeRef indexType = cg_get_intptr_type(ctx);
        LLVMValueRef index = LLVMConstInt(indexType, (unsigned long long)elementOffset, 1);
        result = LLVMConstGEP2(elementType, basePointer, &index, 1);
        if (!result) {
            return NULL;
        }
    }

    if (LLVMTypeOf(result) == targetType) {
        return result;
    }

    LLVMValueRef casted = LLVMConstPointerCast(result, targetType);
    if (!casted) {
        casted = LLVMConstBitCast(result, targetType);
    }
    return casted;
}

LLVMValueRef cg_build_const_initializer(CodegenContext* ctx,
                                        ASTNode* expr,
                                        LLVMTypeRef targetType,
                                        const ParsedType* parsedType) {
#define CG_CONST_INIT_RETURN(value) \
    do {                            \
        profiler_end(scope);        \
        return (value);             \
    } while (0)
    ProfilerScope scope = profiler_begin("codegen_const_initializer");
    profiler_record_value("codegen_count_const_initializer", 1);
    if (!ctx || !expr || !targetType) CG_CONST_INIT_RETURN(NULL);

    LLVMTypeKind targetKind = LLVMGetTypeKind(targetType);
    if (targetKind == LLVMVoidTypeKind) {
        CG_CONST_INIT_RETURN(NULL);
    }

    const ParsedType* effectiveParsed = parsedType;
    if (expr->type == AST_COMPOUND_LITERAL && expr->compoundLiteral.literalType.kind != TYPE_INVALID) {
        effectiveParsed = &expr->compoundLiteral.literalType;
    }

    if (expr->type == AST_COMPOUND_LITERAL &&
        (targetKind == LLVMArrayTypeKind || targetKind == LLVMStructTypeKind)) {
        if (targetKind == LLVMArrayTypeKind) {
            CG_CONST_INIT_RETURN(cg_build_const_array(ctx,
                                                      targetType,
                                                      effectiveParsed,
                                                      expr->compoundLiteral.entries,
                                                      expr->compoundLiteral.entryCount));
        }
        CG_CONST_INIT_RETURN(cg_build_const_struct(ctx,
                                                   targetType,
                                                   effectiveParsed,
                                                   expr->compoundLiteral.entries,
                                                   expr->compoundLiteral.entryCount));
    }

    if (expr->type == AST_COMPOUND_LITERAL && targetKind == LLVMPointerTypeKind) {
        LLVMTypeRef literalType = NULL;
        LLVMValueRef literalPtr = cg_emit_compound_literal_pointer(ctx, expr, &literalType);
        if (!literalPtr || !literalType) {
            CG_CONST_INIT_RETURN(NULL);
        }
        LLVMValueRef result = literalPtr;
        if (LLVMGetTypeKind(literalType) == LLVMArrayTypeKind) {
            LLVMTypeRef i32Ty = LLVMInt32TypeInContext(ctx->llvmContext);
            LLVMValueRef indices[2] = {
                LLVMConstInt(i32Ty, 0, 0),
                LLVMConstInt(i32Ty, 0, 0)
            };
            result = LLVMConstGEP2(literalType, literalPtr, indices, 2);
        }
        if (LLVMTypeOf(result) != targetType) {
            result = LLVMConstBitCast(result, targetType);
        }
        CG_CONST_INIT_RETURN(result);
    }

    if (targetKind == LLVMArrayTypeKind && expr->type == AST_STRING_LITERAL) {
        const char* payload = NULL;
        LiteralEncoding enc = ast_literal_encoding(expr->valueNode.value, &payload);
        const char* text = payload ? payload : expr->valueNode.value;
        unsigned len = LLVMGetArrayLength(targetType);
        LLVMTypeRef elem = LLVMGetElementType(targetType);
        if (!elem || LLVMGetTypeKind(elem) != LLVMIntegerTypeKind) {
            CG_CONST_INIT_RETURN(NULL);
        }
        unsigned elemBits = LLVMGetIntTypeWidth(elem);
        if (enc == LIT_ENC_WIDE && elemBits > 8) {
            uint32_t* codepoints = NULL;
            size_t count = 0;
            decode_c_string_literal_wide(text ? text : "", (int)elemBits, &codepoints, &count);
            LLVMValueRef* chars = (LLVMValueRef*)calloc(len, sizeof(LLVMValueRef));
            if (!chars) {
                free(codepoints);
                CG_CONST_INIT_RETURN(NULL);
            }
            for (unsigned i = 0; i < len; ++i) {
                uint32_t c = 0;
                if (i < count) {
                    c = codepoints[i];
                } else if (i == count) {
                    c = 0;
                }
                chars[i] = LLVMConstInt(elem, c, 0);
            }
            LLVMValueRef arr = LLVMConstArray(elem, chars, len);
            free(chars);
            free(codepoints);
            CG_CONST_INIT_RETURN(arr);
        }

        if (elemBits != 8) {
            CG_CONST_INIT_RETURN(NULL);
        }
        LLVMValueRef* chars = (LLVMValueRef*)calloc(len, sizeof(LLVMValueRef));
        if (!chars) CG_CONST_INIT_RETURN(NULL);
        size_t textLen = text ? strlen(text) : 0;
        for (unsigned i = 0; i < len; ++i) {
            unsigned char c = 0;
            if (i < textLen) {
                c = (unsigned char)text[i];
            } else if (i == textLen) {
                c = 0;
            }
            chars[i] = LLVMConstInt(elem, c, 0);
        }
        LLVMValueRef arr = LLVMConstArray(elem, chars, len);
        free(chars);
        CG_CONST_INIT_RETURN(arr);
    }

    if (targetKind == LLVMPointerTypeKind && expr->type == AST_STRING_LITERAL) {
        const char* payload = NULL;
        LiteralEncoding enc = ast_literal_encoding(expr->valueNode.value, &payload);
        const char* text = payload ? payload : expr->valueNode.value;
        if (enc == LIT_ENC_WIDE) {
            CG_CONST_INIT_RETURN(cg_make_const_wstring_global(ctx, text, targetType));
        }
        CG_CONST_INIT_RETURN(cg_make_const_string_global(ctx, text ? text : "", targetType));
    }

    if (targetKind == LLVMStructTypeKind && expr->type == AST_STRING_LITERAL) {
        CG_CONST_INIT_RETURN(NULL);
    }

    if (targetKind == LLVMFloatTypeKind ||
        targetKind == LLVMDoubleTypeKind ||
        targetKind == LLVMFP128TypeKind) {
        if (expr->type == AST_NUMBER_LITERAL || expr->type == AST_CHAR_LITERAL) {
            const char* text = expr->valueNode.value ? expr->valueNode.value : "0";
            double val = strtod(text, NULL);
            CG_CONST_INIT_RETURN(LLVMConstReal(targetType, val));
        }
        double foldedFP = 0.0;
        if (cg_eval_const_float_expr(expr, &foldedFP)) {
            CG_CONST_INIT_RETURN(LLVMConstReal(targetType, foldedFP));
        }
    }

    if (targetKind == LLVMPointerTypeKind) {
        if (expr->type == AST_BINARY_EXPRESSION &&
            expr->expr.op &&
            expr->expr.left &&
            expr->expr.right &&
            (strcmp(expr->expr.op, "+") == 0 || strcmp(expr->expr.op, "-") == 0)) {
            long long rhsOffset = 0;
            if (cg_eval_initializer_signed_const(ctx, expr->expr.right, &rhsOffset)) {
                long long signedOffset = (strcmp(expr->expr.op, "-") == 0) ? -rhsOffset : rhsOffset;
                LLVMValueRef basePointer = cg_build_const_initializer(ctx,
                                                                      expr->expr.left,
                                                                      targetType,
                                                                      parsedType);
                LLVMValueRef adjusted = cg_const_pointer_with_element_offset(ctx,
                                                                             basePointer,
                                                                             signedOffset,
                                                                             targetType,
                                                                             parsedType);
                if (adjusted) {
                    CG_CONST_INIT_RETURN(adjusted);
                }
            }
            if (strcmp(expr->expr.op, "+") == 0) {
                long long lhsOffset = 0;
                if (cg_eval_initializer_signed_const(ctx, expr->expr.left, &lhsOffset)) {
                    LLVMValueRef basePointer = cg_build_const_initializer(ctx,
                                                                          expr->expr.right,
                                                                          targetType,
                                                                          parsedType);
                    LLVMValueRef adjusted = cg_const_pointer_with_element_offset(ctx,
                                                                                 basePointer,
                                                                                 lhsOffset,
                                                                                 targetType,
                                                                                 parsedType);
                    if (adjusted) {
                        CG_CONST_INIT_RETURN(adjusted);
                    }
                }
            }
        }

        ASTNode* targetExpr = expr;
        bool tookAddress = false;
        if (expr->type == AST_UNARY_EXPRESSION &&
            expr->expr.op &&
            strcmp(expr->expr.op, "&") == 0 &&
            expr->expr.left) {
            targetExpr = expr->expr.left;
            tookAddress = true;
        }
        if (targetExpr && targetExpr->type == AST_IDENTIFIER) {
            const char* name = targetExpr->valueNode.value;
            if (name) {
                LLVMValueRef glob = LLVMGetNamedGlobal(ctx->module, name);
                if (!glob && ctx->semanticModel) {
                    const Symbol* sym = semanticModelLookupGlobal(ctx->semanticModel, name);
                    if (sym && sym->kind == SYMBOL_VARIABLE) {
                        declareGlobalVariableSymbol(ctx, sym);
                        glob = LLVMGetNamedGlobal(ctx->module, name);
                    }
                }
                if (glob) {
                    LLVMValueRef pointerValue = NULL;
                    LLVMTypeRef globalValueType = LLVMGlobalGetValueType(glob);
                    if (!tookAddress &&
                        globalValueType &&
                        LLVMGetTypeKind(globalValueType) == LLVMArrayTypeKind) {
                        LLVMTypeRef i32Ty = LLVMInt32TypeInContext(ctx->llvmContext);
                        LLVMValueRef indices[2] = {
                            LLVMConstInt(i32Ty, 0, 0),
                            LLVMConstInt(i32Ty, 0, 0)
                        };
                        pointerValue = LLVMConstGEP2(globalValueType, glob, indices, 2);
                    } else if (tookAddress) {
                        pointerValue = glob;
                    }
                    if (pointerValue) {
                        if (LLVMTypeOf(pointerValue) == targetType) {
                            return pointerValue;
                        }
                        LLVMValueRef casted = LLVMConstPointerCast(pointerValue, targetType);
                        if (!casted) {
                            casted = LLVMConstBitCast(pointerValue, targetType);
                        }
                        if (casted) {
                            CG_CONST_INIT_RETURN(casted);
                        }
                    }
                }
            }
            if (tookAddress && name && ctx->currentScope) {
                NamedValue* named = cg_scope_lookup(ctx->currentScope, name);
                if (named && named->value && named->isGlobal) {
                    if (LLVMTypeOf(named->value) == targetType) {
                        return named->value;
                    }
                    LLVMValueRef casted = LLVMConstPointerCast(named->value, targetType);
                    if (!casted) {
                        casted = LLVMConstBitCast(named->value, targetType);
                    }
                    if (casted) {
                        CG_CONST_INIT_RETURN(casted);
                    }
                }
            }
            LLVMValueRef fn = name ? LLVMGetNamedFunction(ctx->module, name) : NULL;
            if (!fn && ctx->semanticModel && name) {
                const Symbol* sym = semanticModelLookupGlobal(ctx->semanticModel, name);
                if (sym && sym->kind == SYMBOL_FUNCTION) {
                    declareFunctionSymbol(ctx, sym);
                    fn = LLVMGetNamedFunction(ctx->module, name);
                }
            }
            if (fn) {
                LLVMValueRef casted = LLVMConstPointerCast(fn, targetType);
                if (!casted) {
                    casted = LLVMConstBitCast(fn, targetType);
                }
                CG_CONST_INIT_RETURN(casted);
            }
            if (tookAddress && name) {
                LLVMValueRef glob = LLVMGetNamedGlobal(ctx->module, name);
                if (glob) {
                    LLVMValueRef casted = LLVMConstPointerCast(glob, targetType);
                    if (!casted) {
                        casted = LLVMConstBitCast(glob, targetType);
                    }
                    CG_CONST_INIT_RETURN(casted);
                }
            }
        }

        if (targetExpr && targetExpr->type == AST_ARRAY_ACCESS) {
            LLVMTypeRef accessValueType = NULL;
            LLVMValueRef accessPointer =
                cg_const_resolve_global_lvalue(ctx, targetExpr, &accessValueType);
            if (accessPointer && accessValueType) {
                LLVMValueRef pointerValue = NULL;
                if (!tookAddress &&
                    LLVMGetTypeKind(accessValueType) == LLVMArrayTypeKind) {
                    LLVMTypeRef indexType = cg_get_intptr_type(ctx);
                    LLVMValueRef indices[2] = {
                        LLVMConstInt(indexType, 0, 0),
                        LLVMConstInt(indexType, 0, 0),
                    };
                    pointerValue = LLVMConstGEP2(accessValueType, accessPointer, indices, 2);
                } else if (tookAddress) {
                    pointerValue = accessPointer;
                }

                if (pointerValue) {
                    if (LLVMTypeOf(pointerValue) == targetType) {
                        CG_CONST_INIT_RETURN(pointerValue);
                    }
                    LLVMValueRef casted = LLVMConstPointerCast(pointerValue, targetType);
                    if (!casted) {
                        casted = LLVMConstBitCast(pointerValue, targetType);
                    }
                    if (casted) {
                        CG_CONST_INIT_RETURN(casted);
                    }
                }
            }
        }
    }

    struct Scope* globalScope = NULL;
    if (ctx->semanticModel) {
        globalScope = semanticModelGetGlobalScope(ctx->semanticModel);
    }
    ConstEvalResult res = constEval(expr, globalScope, true);
    if (!res.isConst) {
        CG_CONST_INIT_RETURN(NULL);
    }
    LLVMValueRef folded = cg_const_from_eval(ctx, &res, targetType, effectiveParsed);
    if (folded) {
        CG_CONST_INIT_RETURN(folded);
    }

    CG_CONST_INIT_RETURN(NULL);
#undef CG_CONST_INIT_RETURN
    return NULL;
}
