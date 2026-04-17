// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include "Syntax/const_eval.h"
#include "codegen_types.h"
#include "literal_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_const_string_counter = 0;

static bool cg_is_builtin_const_name(const char* name) {
    if (!name) return false;
    return strcmp(name, "NULL") == 0 ||
           strcmp(name, "true") == 0 ||
           strcmp(name, "false") == 0 ||
           strcmp(name, "__FLT_MIN__") == 0 ||
           strcmp(name, "__DBL_MIN__") == 0 ||
           strcmp(name, "__LDBL_MIN__") == 0;
}

static bool cg_is_builtin_bool_literal_name(const char* name) {
    if (!name) return false;
    return strcmp(name, "true") == 0 || strcmp(name, "false") == 0;
}

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

LLVMValueRef cg_build_const_initializer(CodegenContext* ctx,
                                        ASTNode* expr,
                                        LLVMTypeRef targetType,
                                        const ParsedType* parsedType);
void declareFunctionSymbol(CodegenContext* ctx, const Symbol* sym);

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
    if (!ctx || !expr || !targetType) return NULL;

    LLVMTypeKind targetKind = LLVMGetTypeKind(targetType);
    if (targetKind == LLVMVoidTypeKind) {
        return NULL;
    }

    // Prefer the literal's declared type for compound literals.
    const ParsedType* effectiveParsed = parsedType;
    if (expr->type == AST_COMPOUND_LITERAL && expr->compoundLiteral.literalType.kind != TYPE_INVALID) {
        effectiveParsed = &expr->compoundLiteral.literalType;
    }

    if (expr->type == AST_COMPOUND_LITERAL &&
        (targetKind == LLVMArrayTypeKind || targetKind == LLVMStructTypeKind)) {
        if (targetKind == LLVMArrayTypeKind) {
            return cg_build_const_array(ctx,
                                        targetType,
                                        effectiveParsed,
                                        expr->compoundLiteral.entries,
                                        expr->compoundLiteral.entryCount);
        }
        return cg_build_const_struct(ctx,
                                     targetType,
                                     effectiveParsed,
                                     expr->compoundLiteral.entries,
                                     expr->compoundLiteral.entryCount);
    }

    if (expr->type == AST_COMPOUND_LITERAL && targetKind == LLVMPointerTypeKind) {
        LLVMTypeRef literalType = NULL;
        LLVMValueRef literalPtr = cg_emit_compound_literal_pointer(ctx, expr, &literalType);
        if (!literalPtr || !literalType) {
            return NULL;
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
        return result;
    }

    if (targetKind == LLVMArrayTypeKind && expr->type == AST_STRING_LITERAL) {
        const char* payload = NULL;
        LiteralEncoding enc = ast_literal_encoding(expr->valueNode.value, &payload);
        const char* text = payload ? payload : expr->valueNode.value;
        unsigned len = LLVMGetArrayLength(targetType);
        LLVMTypeRef elem = LLVMGetElementType(targetType);
        if (!elem || LLVMGetTypeKind(elem) != LLVMIntegerTypeKind) {
            return NULL;
        }
        unsigned elemBits = LLVMGetIntTypeWidth(elem);
        if (enc == LIT_ENC_WIDE && elemBits > 8) {
            uint32_t* codepoints = NULL;
            size_t count = 0;
            decode_c_string_literal_wide(text ? text : "", (int)elemBits, &codepoints, &count);
            LLVMValueRef* chars = (LLVMValueRef*)calloc(len, sizeof(LLVMValueRef));
            if (!chars) {
                free(codepoints);
                return NULL;
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
            return arr;
        }

        if (elemBits != 8) {
            return NULL;
        }
        LLVMValueRef* chars = (LLVMValueRef*)calloc(len, sizeof(LLVMValueRef));
        if (!chars) return NULL;
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
        return arr;
    }

    if (targetKind == LLVMPointerTypeKind && expr->type == AST_STRING_LITERAL) {
        const char* payload = NULL;
        LiteralEncoding enc = ast_literal_encoding(expr->valueNode.value, &payload);
        const char* text = payload ? payload : expr->valueNode.value;
        if (enc == LIT_ENC_WIDE) {
            return cg_make_const_wstring_global(ctx, text, targetType);
        }
        return cg_make_const_string_global(ctx, text ? text : "", targetType);
    }

    if (targetKind == LLVMStructTypeKind && expr->type == AST_STRING_LITERAL) {
        return NULL;
    }

    if (targetKind == LLVMFloatTypeKind ||
        targetKind == LLVMDoubleTypeKind ||
        targetKind == LLVMFP128TypeKind) {
        if (expr->type == AST_NUMBER_LITERAL || expr->type == AST_CHAR_LITERAL) {
            const char* text = expr->valueNode.value ? expr->valueNode.value : "0";
            double val = strtod(text, NULL);
            return LLVMConstReal(targetType, val);
        }
        double foldedFP = 0.0;
        if (cg_eval_const_float_expr(expr, &foldedFP)) {
            return LLVMConstReal(targetType, foldedFP);
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
                            return casted;
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
                        return casted;
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
                return casted;
            }
            if (tookAddress && name) {
                LLVMValueRef glob = LLVMGetNamedGlobal(ctx->module, name);
                if (glob) {
                    LLVMValueRef casted = LLVMConstPointerCast(glob, targetType);
                    if (!casted) {
                        casted = LLVMConstBitCast(glob, targetType);
                    }
                    return casted;
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
        return NULL;
    }
    LLVMValueRef folded = cg_const_from_eval(ctx, &res, targetType, effectiveParsed);
    if (folded) {
        return folded;
    }

    return NULL;
}

static DesignatedInit* cg_find_initializer_for_symbol(const Symbol* sym) {
    if (!sym || !sym->definition) {
        return NULL;
    }
    ASTNode* def = sym->definition;
    if (def->type != AST_VARIABLE_DECLARATION) {
        return NULL;
    }
    for (size_t i = 0; i < def->varDecl.varCount; ++i) {
        ASTNode* nameNode = def->varDecl.varNames[i];
        if (nameNode && nameNode->type == AST_IDENTIFIER &&
            nameNode->valueNode.value &&
            strcmp(nameNode->valueNode.value, sym->name) == 0) {
            return def->varDecl.initializers ? def->varDecl.initializers[i] : NULL;
        }
    }
    return NULL;
}

static bool parsedTypeIsPlainVoid(const ParsedType* type) {
    if (!type) return false;
    if (type->kind != TYPE_PRIMITIVE) return false;
    if (type->primitiveType != TOKEN_VOID) return false;
    if (type->pointerDepth != 0) return false;
    if (type->derivationCount != 0) return false;
    return true;
}

static bool parsedTypeIsDirectFunction(const ParsedType* type) {
    return type &&
           type->derivationCount > 0 &&
           type->derivations &&
           type->derivations[0].kind == TYPE_DERIVATION_FUNCTION;
}

static bool cg_parsed_type_is_top_level_const_object(const ParsedType* type) {
    if (!type) return false;

    if (type->derivationCount > 0 && type->derivations) {
        const TypeDerivation* outerPointer = NULL;
        for (size_t i = 0; i < type->derivationCount; ++i) {
            const TypeDerivation* deriv = parsedTypeGetDerivation(type, i);
            if (deriv && deriv->kind == TYPE_DERIVATION_POINTER) {
                outerPointer = deriv;
                break;
            }
        }
        if (outerPointer) {
            return outerPointer->as.pointer.isConst;
        }
        return type->isConst;
    }

    /* Legacy pointerDepth-only forms do not preserve pointer-level qualifiers
     * reliably in ParsedType. Keep this path conservative and writable. */
    if (type->pointerDepth > 0) {
        return false;
    }
    return type->isConst;
}

static void cg_adjust_parameter_type(ParsedType* type) {
    if (!type) return;
    parsedTypeAdjustArrayParameter(type);
    if (!parsedTypeIsDirectFunction(type)) {
        return;
    }
    TypeDerivation* grown = realloc(type->derivations, (type->derivationCount + 1) * sizeof(TypeDerivation));
    if (!grown) {
        return;
    }
    type->derivations = grown;
    memmove(type->derivations + 1, type->derivations, type->derivationCount * sizeof(TypeDerivation));
    memset(&type->derivations[0], 0, sizeof(TypeDerivation));
    type->derivations[0].kind = TYPE_DERIVATION_POINTER;
    type->derivations[0].as.pointer.isConst = false;
    type->derivations[0].as.pointer.isVolatile = false;
    type->derivations[0].as.pointer.isRestrict = false;
    type->derivationCount++;
    type->pointerDepth += 1;
    type->directlyDeclaresFunction = false;
}

static bool paramDeclRepresentsVoid(ASTNode* param) {
    if (!param || param->type != AST_VARIABLE_DECLARATION) {
        return false;
    }
    if (param->varDecl.varCount != 1) {
        return false;
    }
    const ParsedType* parsed = astVarDeclTypeAt(param, 0);
    if (!parsed) {
        parsed = &param->varDecl.declaredType;
    }
    return parsedTypeIsPlainVoid(parsed);
}

size_t cg_expand_parameters(ASTNode** params,
                            size_t paramCount,
                            CGParamInfo** outInfos,
                            bool* outIsVoidList) {
    if (outInfos) {
        *outInfos = NULL;
    }
    if (outIsVoidList) {
        *outIsVoidList = false;
    }
    if (!params || paramCount == 0) {
        return 0;
    }
    if (paramCount == 1 && paramDeclRepresentsVoid(params[0])) {
        if (outIsVoidList) {
            *outIsVoidList = true;
        }
        return 0;
    }

    size_t total = 0;
    for (size_t i = 0; i < paramCount; ++i) {
        ASTNode* paramDecl = params[i];
        if (!paramDecl || paramDecl->type != AST_VARIABLE_DECLARATION) {
            continue;
        }
        total += paramDecl->varDecl.varCount;
    }
    if (total == 0) {
        return 0;
    }

    CGParamInfo* infos = (CGParamInfo*)calloc(total, sizeof(CGParamInfo));
    if (!infos) {
        return 0;
    }

    size_t index = 0;
    for (size_t i = 0; i < paramCount; ++i) {
        ASTNode* paramDecl = params[i];
        if (!paramDecl || paramDecl->type != AST_VARIABLE_DECLARATION) {
            continue;
        }
        for (size_t j = 0; j < paramDecl->varDecl.varCount && index < total; ++j) {
            const ParsedType* parsed = astVarDeclTypeAt(paramDecl, j);
            if (!parsed) {
                parsed = &paramDecl->varDecl.declaredType;
            }
            infos[index].declaration = paramDecl;
            infos[index].nameNode = (paramDecl->varDecl.varNames && j < paramDecl->varDecl.varCount)
                ? paramDecl->varDecl.varNames[j]
                : NULL;
            infos[index].nameIndex = j;
            infos[index].parsedType = parsed;
            ++index;
        }
    }

    if (outInfos) {
        *outInfos = infos;
    } else {
        free(infos);
    }
    return index;
}

void cg_free_param_infos(CGParamInfo* infos) {
    free(infos);
}

LLVMTypeRef* collectParamTypes(CodegenContext* ctx,
                               size_t paramCount,
                               ASTNode** params,
                               size_t* outFlatCount) {
    if (outFlatCount) {
        *outFlatCount = 0;
    }
    CGParamInfo* infos = NULL;
    bool isVoidList = false;
    size_t flatCount = cg_expand_parameters(params, paramCount, &infos, &isVoidList);
    if (outFlatCount) {
        *outFlatCount = flatCount;
    }
    if (isVoidList || flatCount == 0) {
        cg_free_param_infos(infos);
        return NULL;
    }

    LLVMTypeRef* paramTypes = (LLVMTypeRef*)calloc(flatCount, sizeof(LLVMTypeRef));
    if (!paramTypes) {
        cg_free_param_infos(infos);
        return NULL;
    }

    for (size_t i = 0; i < flatCount; ++i) {
        const ParsedType* type = infos[i].parsedType;
        LLVMTypeRef llvmType = cg_lower_parameter_type(ctx, type, NULL, NULL);
        if (!llvmType || LLVMGetTypeKind(llvmType) == LLVMVoidTypeKind) {
            llvmType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        paramTypes[i] = llvmType;
    }

    cg_free_param_infos(infos);
    return paramTypes;
}

static LLVMTypeRef* collectParamTypesFromSignature(CodegenContext* ctx,
                                                   const ParsedType* params,
                                                   size_t paramCount,
                                                   size_t* outFlatCount) {
    if (outFlatCount) {
        *outFlatCount = 0;
    }
    if (!params || paramCount == 0) {
        return NULL;
    }
    if (paramCount == 1 && parsedTypeIsPlainVoid(&params[0])) {
        return NULL;
    }
    LLVMTypeRef* paramTypes = (LLVMTypeRef*)calloc(paramCount, sizeof(LLVMTypeRef));
    if (!paramTypes) {
        return NULL;
    }
    for (size_t i = 0; i < paramCount; ++i) {
        LLVMTypeRef llvmType = cg_lower_parameter_type(ctx, &params[i], NULL, NULL);
        if (!llvmType || LLVMGetTypeKind(llvmType) == LLVMVoidTypeKind) {
            llvmType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        paramTypes[i] = llvmType;
    }
    if (outFlatCount) {
        *outFlatCount = paramCount;
    }
    return paramTypes;
}

LLVMValueRef ensureFunction(CodegenContext* ctx,
                            const char* name,
                            const ParsedType* returnType,
                            size_t paramCount,
                            LLVMTypeRef* paramTypes,
                            bool isVariadic,
                            const Symbol* symHint) {
    if (!ctx || !name) return NULL;

    const TargetLayout* tl = cg_context_get_target_layout(ctx);
    bool supportsStdcall = tl && tl->supportsStdcall;
    bool supportsFastcall = tl && tl->supportsFastcall;
    bool supportsDllStorage = tl && tl->supportsDllStorage;

    LLVMTypeRef returnLLVM = returnType ? cg_type_from_parsed(ctx, returnType) : LLVMVoidTypeInContext(ctx->llvmContext);
    if (returnLLVM && LLVMGetTypeKind(returnLLVM) == LLVMFunctionTypeKind) {
        returnLLVM = LLVMPointerType(returnLLVM, 0);
    } else if (returnLLVM && LLVMGetTypeKind(returnLLVM) == LLVMArrayTypeKind) {
        returnLLVM = LLVMPointerType(returnLLVM, 0);
    }
    if (!returnLLVM || LLVMGetTypeKind(returnLLVM) == LLVMVoidTypeKind) {
        returnLLVM = LLVMVoidTypeInContext(ctx->llvmContext);
    }
    returnLLVM = cg_coerce_function_return_type(ctx, returnLLVM);

    LLVMTypeRef fnType = LLVMFunctionType(returnLLVM, paramTypes, (unsigned)paramCount, isVariadic ? 1 : 0);
    LLVMValueRef existing = LLVMGetNamedFunction(ctx->module, name);
    LLVMValueRef fn = existing ? existing : LLVMAddFunction(ctx->module, name, fnType);

    if (symHint) {
        switch (symHint->signature.callConv) {
            case CALLCONV_STDCALL:
                if (supportsStdcall) {
                    LLVMSetFunctionCallConv(fn, LLVMX86StdcallCallConv);
                }
                break;
            case CALLCONV_FASTCALL:
                if (supportsFastcall) {
                    LLVMSetFunctionCallConv(fn, LLVMX86FastcallCallConv);
                }
                break;
            case CALLCONV_CDECL:
                LLVMSetFunctionCallConv(fn, LLVMCCallConv);
                break;
            case CALLCONV_DEFAULT:
            default:
                break;
        }
        if (supportsDllStorage) {
            if (symHint->dllStorage == DLL_STORAGE_EXPORT) {
                LLVMSetDLLStorageClass(fn, LLVMDLLExportStorageClass);
            } else if (symHint->dllStorage == DLL_STORAGE_IMPORT) {
                LLVMSetDLLStorageClass(fn, LLVMDLLImportStorageClass);
            }
        }
    }

    if (ctx->verifyFunctions) {
        char* error = NULL;
        LLVMVerifyFunction(fn, LLVMAbortProcessAction);
        if (error) {
            LLVMDisposeMessage(error);
        }
    }
    return fn;
}

void declareFunctionPrototype(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node) return;

    const ParsedType* returnType = NULL;
    const char* name = NULL;
    size_t paramCount = 0;
    ASTNode** params = NULL;
    bool isVariadic = false;

    switch (node->type) {
        case AST_FUNCTION_DEFINITION:
            if (!node->functionDef.funcName || node->functionDef.funcName->type != AST_IDENTIFIER) return;
            name = node->functionDef.funcName->valueNode.value;
            returnType = &node->functionDef.returnType;
            paramCount = node->functionDef.paramCount;
            params = node->functionDef.parameters;
            isVariadic = node->functionDef.isVariadic;
            break;
        case AST_FUNCTION_DECLARATION:
            if (!node->functionDecl.funcName || node->functionDecl.funcName->type != AST_IDENTIFIER) return;
            name = node->functionDecl.funcName->valueNode.value;
            returnType = &node->functionDecl.returnType;
            paramCount = node->functionDecl.paramCount;
            params = node->functionDecl.parameters;
            isVariadic = node->functionDecl.isVariadic;
            break;
        default:
            return;
    }

    if (!name) return;

    size_t flattenedCount = 0;
    LLVMTypeRef* paramTypes = collectParamTypes(ctx, paramCount, params, &flattenedCount);
    LLVMValueRef fn = ensureFunction(ctx, name, returnType, flattenedCount, paramTypes, isVariadic, NULL);
    (void)fn;
    free(paramTypes);
}

void declareGlobalVariableSymbol(CodegenContext* ctx, const Symbol* sym) {
    if (!ctx || !sym || !sym->name) return;

    /* `true`/`false` are compiler-seeded constant identifiers.
     * Keep them as immediate constants; do not materialize mutable globals. */
    if (!sym->definition &&
        sym->hasConstValue &&
        cg_is_builtin_bool_literal_name(sym->name)) {
        return;
    }

    LLVMTypeRef varType = cg_type_from_parsed(ctx, &sym->type);
    if (!varType || LLVMGetTypeKind(varType) == LLVMVoidTypeKind) {
        varType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    bool isArray = parsedTypeIsDirectArray(&sym->type);
    LLVMTypeRef elementLLVM = NULL;
    if (isArray) {
        ParsedType element = parsedTypeArrayElementType(&sym->type);
        elementLLVM = cg_type_from_parsed(ctx, &element);
        parsedTypeFree(&element);
        if (!elementLLVM || LLVMGetTypeKind(elementLLVM) == LLVMVoidTypeKind) {
            elementLLVM = LLVMInt32TypeInContext(ctx->llvmContext);
        }
    }

    bool externDeclOnly = (sym->storage == STORAGE_EXTERN &&
                           !sym->hasDefinition &&
                           !sym->isTentative);

    LLVMValueRef existing = LLVMGetNamedGlobal(ctx->module, sym->name);
    if (!existing) {
        existing = LLVMAddGlobal(ctx->module, varType, sym->name);
        if (!externDeclOnly) {
            LLVMSetInitializer(existing, LLVMConstNull(varType));
        }
    }

    if (externDeclOnly) {
        LLVMSetLinkage(existing, LLVMExternalLinkage);
    } else if (cg_is_builtin_const_name(sym->name)) {
        LLVMSetLinkage(existing, LLVMInternalLinkage);
    } else if (sym->isTentative) {
        LLVMSetLinkage(existing, LLVMCommonLinkage);
    } else if (sym->linkage == LINKAGE_INTERNAL) {
        LLVMSetLinkage(existing, LLVMInternalLinkage);
    } else {
        LLVMSetLinkage(existing, LLVMExternalLinkage);
    }

    if (!externDeclOnly && !sym->isTentative) {
        DesignatedInit* init = cg_find_initializer_for_symbol(sym);
        const char* skipConst = getenv("FISICS_NO_CONST_GLOBALS");
        const char* dbg = getenv("FISICS_DEBUG_CONST");
        if (dbg) {
            if (!init) {
                fprintf(stderr, "[const-init] no initializer found for %s\n", sym->name);
            } else if (!init->expression) {
                fprintf(stderr, "[const-init] no expression for %s\n", sym->name);
            }
        }
        if (!skipConst && init && init->expression && !parsedTypeHasVLA(&sym->type)) {
            LLVMTypeRef elementType = LLVMGlobalGetValueType(existing);
            if (dbg) {
                fprintf(stderr,
                        "[const-init] %s targetKind=%d exprType=%d\n",
                        sym->name,
                        elementType ? (int)LLVMGetTypeKind(elementType) : -1,
                        init->expression ? (int)init->expression->type : -1);
            }
            if (elementType) {
                LLVMValueRef constInit = cg_build_const_initializer(ctx,
                                                                    init->expression,
                                                                    elementType,
                                                                    &sym->type);
                if (constInit) {
                    LLVMSetInitializer(existing, constInit);
                    if (cg_parsed_type_is_top_level_const_object(&sym->type) &&
                        !sym->type.isVolatile) {
                        LLVMSetGlobalConstant(existing, 1);
                        LLVMSetUnnamedAddr(existing, LLVMGlobalUnnamedAddr);
                    }
                    if (sym->linkage == LINKAGE_INTERNAL) {
                        LLVMSetLinkage(existing, LLVMInternalLinkage);
                    }
                }
            }
        }
    }

    uint64_t szDummy = 0;
    uint32_t alignVal = 0;
    /* Global predeclare must stay resilient for incomplete/opaque declarations.
     * Only pass an LLVM hint when the lowered type is sized. */
    LLVMTypeRef alignHint = (varType && LLVMTypeIsSized(varType)) ? varType : NULL;
    if (cg_size_align_for_type(ctx, &sym->type, alignHint, &szDummy, &alignVal) && alignVal > 0) {
        LLVMSetAlignment(existing, alignVal);
    }
    if (sym->dllStorage == DLL_STORAGE_EXPORT) {
        LLVMSetDLLStorageClass(existing, LLVMDLLExportStorageClass);
    } else if (sym->dllStorage == DLL_STORAGE_IMPORT) {
        LLVMSetDLLStorageClass(existing, LLVMDLLImportStorageClass);
    }

    cg_scope_insert(ctx->currentScope,
                    sym->name,
                    existing,
                    varType,
                    true,
                    isArray,
                    elementLLVM,
                    &sym->type);
}

void declareFunctionSymbol(CodegenContext* ctx, const Symbol* sym) {
    if (!ctx || !sym || !sym->name) return;

    size_t paramCount = 0;
    ASTNode** params = NULL;
    const ParsedType* sigParams = NULL;
    bool useSignature = false;
    if (sym->definition) {
        if (sym->definition->type == AST_FUNCTION_DEFINITION) {
            paramCount = sym->definition->functionDef.paramCount;
            params = sym->definition->functionDef.parameters;
        } else if (sym->definition->type == AST_FUNCTION_DECLARATION) {
            paramCount = sym->definition->functionDecl.paramCount;
            params = sym->definition->functionDecl.parameters;
        }
    } else if (sym->signature.paramCount > 0 && sym->signature.params) {
        paramCount = sym->signature.paramCount;
        sigParams = sym->signature.params;
        useSignature = true;
    }

    size_t flattenedCount = 0;
    LLVMTypeRef* paramTypes = NULL;
    if (useSignature) {
        paramTypes = collectParamTypesFromSignature(ctx, sigParams, paramCount, &flattenedCount);
    } else {
        paramTypes = collectParamTypes(ctx, paramCount, params, &flattenedCount);
    }
    LLVMValueRef fn = ensureFunction(ctx,
                                     sym->name,
                                     &sym->type,
                                     flattenedCount,
                                     paramTypes,
                                     sym->signature.isVariadic,
                                     sym);
    (void)fn;
    free(paramTypes);
}

static void predeclareGlobalSymbolCallback(const Symbol* sym, void* userData) {
    CodegenContext* ctx = (CodegenContext*)userData;
    if (!ctx || !sym) return;
    switch (sym->kind) {
        case SYMBOL_VARIABLE:
            declareGlobalVariableSymbol(ctx, sym);
            break;
        case SYMBOL_FUNCTION:
            declareFunctionSymbol(ctx, sym);
            break;
        case SYMBOL_STRUCT:
            declareStructSymbol(ctx, sym);
            break;
        case SYMBOL_ENUM:
            break;
        default:
            break;
    }
}

void declareGlobalVariable(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_VARIABLE_DECLARATION) return;

    for (size_t i = 0; i < node->varDecl.varCount; ++i) {
        ASTNode* varNameNode = node->varDecl.varNames[i];
        if (!varNameNode || varNameNode->type != AST_IDENTIFIER) continue;
        const char* name = varNameNode->valueNode.value;
        if (!name) continue;
        const ParsedType* parsedType = astVarDeclTypeAt(node, i);
        const ParsedType* effectiveParsed = parsedType ? parsedType : &node->varDecl.declaredType;
        const ParsedType* resolvedParsed = cg_resolve_typedef_parsed(ctx, effectiveParsed);
        const ParsedType* arrayParsed = (resolvedParsed && parsedTypeIsDirectArray(resolvedParsed))
            ? resolvedParsed
            : effectiveParsed;
        LLVMTypeRef varType = cg_type_from_parsed(ctx, effectiveParsed);
        if (!varType || LLVMGetTypeKind(varType) == LLVMVoidTypeKind) {
            varType = LLVMInt32TypeInContext(ctx->llvmContext);
        }

        bool isArray = arrayParsed ? parsedTypeIsDirectArray(arrayParsed) : false;
        LLVMValueRef existing = LLVMGetNamedGlobal(ctx->module, name);
        if (existing) {
            LLVMTypeRef existingType = cg_dereference_ptr_type(ctx, LLVMTypeOf(existing), "global variable");
            if (!existingType || LLVMGetTypeKind(existingType) == LLVMVoidTypeKind) {
                existingType = varType;
            }
            LLVMTypeRef elementLLVM = NULL;
            if (isArray) {
                ParsedType element = parsedTypeArrayElementType(arrayParsed);
                elementLLVM = cg_type_from_parsed(ctx, &element);
                parsedTypeFree(&element);
                if (!elementLLVM || LLVMGetTypeKind(elementLLVM) == LLVMVoidTypeKind) {
                    elementLLVM = LLVMInt32TypeInContext(ctx->llvmContext);
                }
            }
        if (cg_is_builtin_const_name(name)) {
            LLVMSetLinkage(existing, LLVMInternalLinkage);
        }
        const ParsedType* storedParsed = parsedType ? parsedType : &node->varDecl.declaredType;
        if (storedParsed && storedParsed->kind == TYPE_NAMED) {
            CGTypeCache* cache = cg_context_get_type_cache(ctx);
            CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, storedParsed->userTypeName);
            if (info) {
                storedParsed = &info->parsedType;
            }
        }
        cg_scope_insert(ctx->currentScope,
                        name,
                        existing,
                        existingType,
                        true,
                        isArray,
                        elementLLVM,
                        storedParsed);
            continue;
        }

        LLVMValueRef global = LLVMAddGlobal(ctx->module, varType, name);
        if (cg_is_builtin_const_name(name)) {
            LLVMSetLinkage(global, LLVMInternalLinkage);
        }
        LLVMSetInitializer(global, LLVMConstNull(varType));
        LLVMTypeRef elementLLVM = NULL;
        if (isArray) {
            ParsedType element = parsedTypeArrayElementType(arrayParsed);
            elementLLVM = cg_type_from_parsed(ctx, &element);
            parsedTypeFree(&element);
            if (!elementLLVM || LLVMGetTypeKind(elementLLVM) == LLVMVoidTypeKind) {
                elementLLVM = LLVMInt32TypeInContext(ctx->llvmContext);
            }
        }
        const ParsedType* storedParsed = parsedType ? parsedType : &node->varDecl.declaredType;
        if (storedParsed && storedParsed->kind == TYPE_NAMED) {
            CGTypeCache* cache = cg_context_get_type_cache(ctx);
            CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, storedParsed->userTypeName);
            if (info) {
                storedParsed = &info->parsedType;
            }
        }
        cg_scope_insert(ctx->currentScope,
                        name,
                        global,
                        varType,
                        true,
                        isArray,
                        elementLLVM,
                        storedParsed);
    }
}

void predeclareGlobals(CodegenContext* ctx, ASTNode* program) {
    if (!ctx) return;

    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (model) {
        semanticModelForEachGlobal(model, predeclareGlobalSymbolCallback, ctx);

        if (program && program->type == AST_PROGRAM) {
            for (size_t i = 0; i < program->block.statementCount; ++i) {
                ASTNode* stmt = program->block.statements[i];
                if (!stmt) continue;
                if (stmt->type == AST_STRUCT_DEFINITION || stmt->type == AST_UNION_DEFINITION) {
                    (void)codegenStructDefinition(ctx, stmt);
                } else if (stmt->type == AST_TYPEDEF &&
                           stmt->typedefStmt.baseType.inlineStructOrUnionDef) {
                    (void)codegenStructDefinition(ctx, stmt->typedefStmt.baseType.inlineStructOrUnionDef);
                }
            }
        }
        return;
    }

    if (!program || program->type != AST_PROGRAM) return;

    for (size_t i = 0; i < program->block.statementCount; ++i) {
        ASTNode* stmt = program->block.statements[i];
        if (!stmt) continue;
        switch (stmt->type) {
            case AST_VARIABLE_DECLARATION:
                declareGlobalVariable(ctx, stmt);
                break;
            case AST_FUNCTION_DECLARATION:
            case AST_FUNCTION_DEFINITION:
                declareFunctionPrototype(ctx, stmt);
                break;
            case AST_STRUCT_DEFINITION:
            case AST_UNION_DEFINITION:
                (void)codegenStructDefinition(ctx, stmt);
                break;
            case AST_TYPEDEF:
                if (stmt->typedefStmt.baseType.inlineStructOrUnionDef) {
                    (void)codegenStructDefinition(ctx, stmt->typedefStmt.baseType.inlineStructOrUnionDef);
                }
                break;
            default:
                break;
        }
    }
}
