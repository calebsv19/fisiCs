// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include "Syntax/const_eval.h"
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

static const ParsedType* cg_resolve_typedef_parsed(CodegenContext* ctx, const ParsedType* type) {
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

static unsigned long long cg_eval_initializer_index_const(CodegenContext* ctx,
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

static bool cg_entries_flat_scalars(DesignatedInit** entries, size_t entryCount) {
    if (!entries || entryCount == 0) return false;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;
        if (entry->indexExpr || entry->fieldName) {
            return false;
        }
        if (entry->expression->type == AST_COMPOUND_LITERAL) {
            return false;
        }
    }
    return true;
}

static LLVMValueRef cg_build_const_array_flat(CodegenContext* ctx,
                                              LLVMTypeRef arrayType,
                                              const ParsedType* parsedType,
                                              DesignatedInit** entries,
                                              size_t entryCount,
                                              size_t* cursor) {
    if (!ctx || !arrayType || LLVMGetTypeKind(arrayType) != LLVMArrayTypeKind || !cursor) {
        return NULL;
    }

    unsigned length = LLVMGetArrayLength(arrayType);
    LLVMTypeRef elemType = LLVMGetElementType(arrayType);
    if (!elemType) return NULL;

    LLVMValueRef* values = (LLVMValueRef*)calloc(length, sizeof(LLVMValueRef));
    if (!values) return NULL;

    ParsedType elementParsed = {0};
    bool hasElementParsed = false;
    if (parsedType && parsedTypeIsDirectArray(parsedType)) {
        elementParsed = parsedTypeArrayElementType(parsedType);
        hasElementParsed = true;
    }

    for (unsigned i = 0; i < length; ++i) {
        LLVMValueRef elemConst = cg_zero_const(elemType);
        if (*cursor < entryCount) {
            DesignatedInit* entry = entries[*cursor];
            if (entry && entry->expression) {
                if (LLVMGetTypeKind(elemType) == LLVMArrayTypeKind) {
                    if (entry->expression->type == AST_COMPOUND_LITERAL) {
                        elemConst = cg_build_const_initializer(ctx,
                                                               entry->expression,
                                                               elemType,
                                                               hasElementParsed ? &elementParsed : NULL);
                        (*cursor)++;
                    } else {
                        elemConst = cg_build_const_array_flat(ctx,
                                                              elemType,
                                                              hasElementParsed ? &elementParsed : NULL,
                                                              entries,
                                                              entryCount,
                                                              cursor);
                    }
                } else {
                    elemConst = cg_build_const_initializer(ctx,
                                                           entry->expression,
                                                           elemType,
                                                           hasElementParsed ? &elementParsed : NULL);
                    (*cursor)++;
                }
            }
        }
        if (!elemConst) {
            elemConst = cg_zero_const(elemType);
        }
        values[i] = elemConst;
    }

    LLVMValueRef result = LLVMConstArray(elemType, values, length);
    free(values);
    if (hasElementParsed) {
        parsedTypeFree(&elementParsed);
    }
    return result;
}

static LLVMValueRef cg_build_const_array(CodegenContext* ctx,
                                         LLVMTypeRef arrayType,
                                         const ParsedType* parsedType,
                                         DesignatedInit** entries,
                                         size_t entryCount) {
    if (!ctx || !arrayType || LLVMGetTypeKind(arrayType) != LLVMArrayTypeKind) return NULL;

    unsigned length = LLVMGetArrayLength(arrayType);
    LLVMTypeRef elemType = LLVMGetElementType(arrayType);
    if (!elemType) return NULL;

    if (LLVMGetTypeKind(elemType) == LLVMArrayTypeKind &&
        cg_entries_flat_scalars(entries, entryCount)) {
        size_t cursor = 0;
        return cg_build_const_array_flat(ctx, arrayType, parsedType, entries, entryCount, &cursor);
    }

    LLVMValueRef* values = (LLVMValueRef*)calloc(length, sizeof(LLVMValueRef));
    if (!values) return NULL;
    for (unsigned i = 0; i < length; ++i) {
        values[i] = cg_zero_const(elemType);
    }

    ParsedType elementParsed = {0};
    if (parsedType && parsedTypeIsDirectArray(parsedType)) {
        elementParsed = parsedTypeArrayElementType(parsedType);
    }

    unsigned long long implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;
        unsigned long long targetIndex = implicitIndex;
        if (entry->indexExpr) {
            bool ok = false;
            targetIndex = cg_eval_initializer_index_const(ctx, entry->indexExpr, &ok);
            if (!ok) {
                free(values);
                if (parsedType && parsedTypeIsDirectArray(parsedType)) {
                    parsedTypeFree(&elementParsed);
                }
                return NULL;
            }
        }
        implicitIndex = targetIndex + 1;
        if (targetIndex >= length) {
            continue;
        }
        LLVMValueRef elementConst = cg_build_const_initializer(ctx,
                                                               entry->expression,
                                                               elemType,
                                                               parsedType ? &elementParsed : NULL);
        if (!elementConst) {
            free(values);
            if (parsedType && parsedTypeIsDirectArray(parsedType)) {
                parsedTypeFree(&elementParsed);
            }
            return NULL;
        }
        values[targetIndex] = elementConst;
    }

    LLVMValueRef result = LLVMConstArray(elemType, values, length);
    free(values);
    if (parsedType && parsedTypeIsDirectArray(parsedType)) {
        parsedTypeFree(&elementParsed);
    }
    return result;
}

static bool cg_find_field_in_definition(const ASTNode* def,
                                        const char* fieldName,
                                        unsigned* outIndex,
                                        const ParsedType** outParsed) {
    if (!def || !fieldName || !outIndex) return false;
    if (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION) return false;

    unsigned index = 0;
    for (size_t f = 0; f < def->structDef.fieldCount; ++f) {
        const ASTNode* fieldDecl = def->structDef.fields[f];
        if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
        for (size_t v = 0; v < fieldDecl->varDecl.varCount; ++v) {
            const ASTNode* nameNode = fieldDecl->varDecl.varNames[v];
            const char* candidate = (nameNode && nameNode->type == AST_IDENTIFIER)
                ? nameNode->valueNode.value
                : NULL;
            if (candidate && strcmp(candidate, fieldName) == 0) {
                *outIndex = (def->type == AST_UNION_DEFINITION) ? 0u : index;
                if (outParsed) {
                    const ParsedType* parsed = astVarDeclTypeAt((ASTNode*)fieldDecl, v);
                    *outParsed = parsed ? parsed : &fieldDecl->varDecl.declaredType;
                }
                return true;
            }
            if (def->type != AST_UNION_DEFINITION) {
                ++index;
            }
        }
    }
    return false;
}

static LLVMValueRef cg_build_const_struct(CodegenContext* ctx,
                                          LLVMTypeRef structType,
                                          const ParsedType* parsedType,
                                          DesignatedInit** entries,
                                          size_t entryCount) {
    if (!ctx || !structType || LLVMGetTypeKind(structType) != LLVMStructTypeKind) return NULL;
    const char* dbg = getenv("FISICS_DEBUG_CONST");

    const ParsedType* resolvedType = parsedType ? cg_resolve_typedef_parsed(ctx, parsedType) : parsedType;
    const ParsedType* lookupType = resolvedType ? resolvedType : parsedType;
    bool isUnion = lookupType && lookupType->kind == TYPE_UNION;
    CGStructLLVMInfo* info = cg_find_struct_info(ctx, structType, lookupType);
    if (info && info->isUnion) {
        isUnion = true;
    }

    unsigned fieldCount = LLVMCountStructElementTypes(structType);
    if (fieldCount == 0 && parsedType) {
        LLVMTypeRef resolved = cg_type_from_parsed(ctx, parsedType);
        if (resolved) {
            structType = resolved;
            fieldCount = LLVMCountStructElementTypes(structType);
        }
    }
    if (fieldCount == 0 && info && info->definition) {
        (void)codegenStructDefinition(ctx, (ASTNode*)info->definition);
        fieldCount = LLVMCountStructElementTypes(structType);
    }
    if (fieldCount == 0 && info && info->fieldCount > 0) {
        LLVMTypeRef* fieldTypes = (LLVMTypeRef*)calloc(info->fieldCount, sizeof(LLVMTypeRef));
        if (fieldTypes) {
            for (size_t i = 0; i < info->fieldCount; ++i) {
                fieldTypes[i] = cg_type_from_parsed(ctx, &info->fields[i].parsedType);
            }
            LLVMStructSetBody(structType, fieldTypes, (unsigned)info->fieldCount, 0);
            free(fieldTypes);
            fieldCount = LLVMCountStructElementTypes(structType);
        }
    }
    if (!info && lookupType && lookupType->userTypeName && ctx->semanticModel) {
        CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
        if (cctx) {
            CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
            ASTNode* def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
            if (def) {
                (void)codegenStructDefinition(ctx, def);
                info = cg_find_struct_info(ctx, structType, lookupType);
            }
        }
    }

    if (fieldCount == 0 && parsedType && parsedType->userTypeName && ctx->semanticModel) {
        CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
        if (cctx) {
            CCTagKind kind = (parsedType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
            ASTNode* def = cc_tag_definition(cctx, kind, parsedType->userTypeName);
            if (def) {
                (void)codegenStructDefinition(ctx, def);
                structType = cg_type_from_parsed(ctx, parsedType);
                fieldCount = LLVMCountStructElementTypes(structType);
            }
        }
    }
    if (fieldCount == 0) {
        if (dbg) {
            const char* name = parsedType ? parsedType->userTypeName : NULL;
            fprintf(stderr,
                    "[const-init] struct still opaque (entries=%zu, name=%s, info=%s)\n",
                    entryCount,
                    name ? name : "<anon>",
                    info ? "yes" : "no");
        }
        return LLVMConstStruct(NULL, 0, 0);
    }

    if (dbg) {
        fprintf(stderr, "[const-init] struct fields=%u entries=%zu isUnion=%d\n",
                fieldCount,
                entryCount,
                isUnion ? 1 : 0);
    }

    LLVMValueRef* fields = (LLVMValueRef*)calloc(fieldCount, sizeof(LLVMValueRef));
    if (!fields) return NULL;
    for (unsigned i = 0; i < fieldCount; ++i) {
        fields[i] = cg_zero_const(LLVMStructGetTypeAtIndex(structType, i));
    }

    if (isUnion) {
        if (entryCount > 0 && entries[0] && entries[0]->expression) {
            LLVMTypeRef firstType = LLVMStructGetTypeAtIndex(structType, 0);
            const ParsedType* fieldParsed = NULL;
            LLVMTypeRef fieldType = firstType;
            if (info && info->fieldCount > 0) {
                fieldParsed = &info->fields[0].parsedType;
            }
            if (entries[0]->fieldName && info) {
                for (size_t f = 0; f < info->fieldCount; ++f) {
                    const char* fname = info->fields[f].name;
                    if (fname && strcmp(fname, entries[0]->fieldName) == 0) {
                        fieldParsed = &info->fields[f].parsedType;
                        fieldType = cg_type_from_parsed(ctx, fieldParsed);
                        if (!fieldType) {
                            fieldType = firstType;
                        }
                        break;
                    }
                }
            }
            if (entries[0]->fieldName && !info && lookupType && lookupType->userTypeName && ctx->semanticModel) {
                CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                if (cctx) {
                    CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                    ASTNode* def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
                    if (def && (def->type == AST_UNION_DEFINITION || def->type == AST_STRUCT_DEFINITION)) {
                        for (size_t f = 0; f < def->structDef.fieldCount; ++f) {
                            ASTNode* fieldDecl = def->structDef.fields[f];
                            if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
                            for (size_t v = 0; v < fieldDecl->varDecl.varCount; ++v) {
                                ASTNode* nameNode = fieldDecl->varDecl.varNames[v];
                                const char* fname = (nameNode && nameNode->type == AST_IDENTIFIER)
                                    ? nameNode->valueNode.value
                                    : NULL;
                                if (fname && strcmp(fname, entries[0]->fieldName) == 0) {
                                    const ParsedType* parsed = astVarDeclTypeAt(fieldDecl, v);
                                    fieldParsed = parsed ? parsed : &fieldDecl->varDecl.declaredType;
                                    fieldType = cg_type_from_parsed(ctx, fieldParsed);
                                    if (!fieldType) {
                                        fieldType = firstType;
                                    }
                                    f = def->structDef.fieldCount;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            LLVMValueRef val = cg_build_const_initializer(ctx, entries[0]->expression, fieldType, fieldParsed);
            if (val && LLVMTypeOf(val) != firstType) {
                val = LLVMConstBitCast(val, firstType);
            }
            if (val) {
                fields[0] = val;
            }
        }
        LLVMValueRef result = LLVMConstNamedStruct(structType, fields, fieldCount);
        free(fields);
        return result;
    }

    unsigned implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned targetIndex = implicitIndex;
        const ParsedType* fieldParsed = NULL;

        bool matchedField = false;
        if (entry->fieldName && info && info->fieldCount > 0) {
            for (size_t f = 0; f < info->fieldCount; ++f) {
                if (info->fields[f].name && strcmp(info->fields[f].name, entry->fieldName) == 0) {
                    targetIndex = info->fields[f].index;
                    fieldParsed = &info->fields[f].parsedType;
                    matchedField = true;
                    break;
                }
            }
        }
        if (entry->fieldName && !matchedField) {
            const ASTNode* def = info ? (const ASTNode*)info->definition : NULL;
            if (!def && lookupType && lookupType->userTypeName && ctx->semanticModel) {
                CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                if (cctx) {
                    CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                    def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
                }
            }
            if (def) {
                matchedField = cg_find_field_in_definition(def, entry->fieldName, &targetIndex, &fieldParsed);
            }
        }
        implicitIndex = targetIndex + 1;

        if (dbg) {
            fprintf(stderr, "[const-init] struct entry idx=%u field=%s\n",
                    targetIndex,
                    entry->fieldName ? entry->fieldName : "<implicit>");
        }

        if (targetIndex >= fieldCount) {
            continue;
        }

        LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(structType, targetIndex);
        LLVMValueRef fieldConst = cg_build_const_initializer(ctx, entry->expression, fieldType, fieldParsed);
        if (!fieldConst) {
            free(fields);
            return NULL;
        }
        fields[targetIndex] = fieldConst;
    }

    LLVMValueRef result = LLVMConstNamedStruct(structType, fields, fieldCount);
    free(fields);
    return result;
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
                if (getenv("FISICS_DEBUG_CONST")) {
                    char* printed = LLVMPrintValueToString(casted);
                    char* castType = casted ? LLVMPrintTypeToString(LLVMTypeOf(casted)) : NULL;
                    char* targetStr = LLVMPrintTypeToString(targetType);
                    fprintf(stderr, "[const-init] fn %s -> %s\n", name ? name : "<null>", printed ? printed : "<null>");
                    fprintf(stderr, "[const-init] fn type=%s target=%s\n",
                            castType ? castType : "<null>",
                            targetStr ? targetStr : "<null>");
                    if (printed) LLVMDisposeMessage(printed);
                    if (castType) LLVMDisposeMessage(castType);
                    if (targetStr) LLVMDisposeMessage(targetStr);
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
}
