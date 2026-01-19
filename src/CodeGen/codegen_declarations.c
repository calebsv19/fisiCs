#include "codegen_private.h"

#include "Syntax/const_eval.h"
#include "codegen_types.h"
#include "literal_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_const_string_counter = 0;

static const ParsedType* cg_resolve_typedef_parsed(CodegenContext* ctx, const ParsedType* type) {
    if (!ctx || !type || type->kind != TYPE_NAMED || !type->userTypeName) {
        return type;
    }
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (!cache) return type;
    CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, type->userTypeName);
    if (info) {
        return &info->parsedType;
    }
    return type;
}

LLVMValueRef cg_build_const_initializer(CodegenContext* ctx,
                                        ASTNode* expr,
                                        LLVMTypeRef targetType,
                                        const ParsedType* parsedType);

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

static LLVMValueRef cg_build_const_array(CodegenContext* ctx,
                                         LLVMTypeRef arrayType,
                                         const ParsedType* parsedType,
                                         DesignatedInit** entries,
                                         size_t entryCount) {
    if (!ctx || !arrayType || LLVMGetTypeKind(arrayType) != LLVMArrayTypeKind) return NULL;

    unsigned length = LLVMGetArrayLength(arrayType);
    LLVMTypeRef elemType = LLVMGetElementType(arrayType);
    if (!elemType) return NULL;

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
        } else {
            implicitIndex = targetIndex + 1;
        }
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

static LLVMValueRef cg_build_const_struct(CodegenContext* ctx,
                                          LLVMTypeRef structType,
                                          const ParsedType* parsedType,
                                          DesignatedInit** entries,
                                          size_t entryCount) {
    if (!ctx || !structType || LLVMGetTypeKind(structType) != LLVMStructTypeKind) return NULL;
    const char* dbg = getenv("FISICS_DEBUG_CONST");

    bool isUnion = parsedType && parsedType->kind == TYPE_UNION;
    CGStructLLVMInfo* info = cg_find_struct_info(ctx, structType, parsedType);
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
            const ParsedType* firstParsed = NULL;
            if (info && info->fieldCount > 0) {
                firstParsed = &info->fields[0].parsedType;
            }
            LLVMValueRef val = cg_build_const_initializer(ctx, entries[0]->expression, firstType, firstParsed);
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

        if (entry->fieldName && info) {
            for (size_t f = 0; f < info->fieldCount; ++f) {
                if (info->fields[f].name && strcmp(info->fields[f].name, entry->fieldName) == 0) {
                    targetIndex = info->fields[f].index;
                    fieldParsed = &info->fields[f].parsedType;
                    break;
                }
            }
        } else {
            implicitIndex = targetIndex + 1;
        }

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
        LLVMTypeRef llvmType = cg_type_from_parsed(ctx, type);
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
        LLVMTypeRef llvmType = cg_type_from_parsed(ctx, &params[i]);
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
                            const Symbol* symHint) {
    if (!ctx || !name) return NULL;

    const TargetLayout* tl = cg_context_get_target_layout(ctx);
    bool supportsStdcall = tl && tl->supportsStdcall;
    bool supportsFastcall = tl && tl->supportsFastcall;
    bool supportsDllStorage = tl && tl->supportsDllStorage;

    LLVMTypeRef returnLLVM = returnType ? cg_type_from_parsed(ctx, returnType) : LLVMVoidTypeInContext(ctx->llvmContext);
    if (!returnLLVM || LLVMGetTypeKind(returnLLVM) == LLVMVoidTypeKind) {
        returnLLVM = LLVMVoidTypeInContext(ctx->llvmContext);
    }

    LLVMTypeRef fnType = LLVMFunctionType(returnLLVM, paramTypes, (unsigned)paramCount, 0);
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

    switch (node->type) {
        case AST_FUNCTION_DEFINITION:
            if (!node->functionDef.funcName || node->functionDef.funcName->type != AST_IDENTIFIER) return;
            name = node->functionDef.funcName->valueNode.value;
            returnType = &node->functionDef.returnType;
            paramCount = node->functionDef.paramCount;
            params = node->functionDef.parameters;
            break;
        case AST_FUNCTION_DECLARATION:
            if (!node->functionDecl.funcName || node->functionDecl.funcName->type != AST_IDENTIFIER) return;
            name = node->functionDecl.funcName->valueNode.value;
            returnType = &node->functionDecl.returnType;
            paramCount = node->functionDecl.paramCount;
            params = node->functionDecl.parameters;
            break;
        default:
            return;
    }

    if (!name) return;

    size_t flattenedCount = 0;
    LLVMTypeRef* paramTypes = collectParamTypes(ctx, paramCount, params, &flattenedCount);
    LLVMValueRef fn = ensureFunction(ctx, name, returnType, flattenedCount, paramTypes, NULL);
    (void)fn;
    free(paramTypes);
}

void declareGlobalVariableSymbol(CodegenContext* ctx, const Symbol* sym) {
    if (!ctx || !sym || !sym->name) return;

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

    LLVMValueRef existing = LLVMGetNamedGlobal(ctx->module, sym->name);
    if (!existing) {
        existing = LLVMAddGlobal(ctx->module, varType, sym->name);
        LLVMSetInitializer(existing, LLVMConstNull(varType));
    }

    if (!sym->isTentative) {
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
                    LLVMSetGlobalConstant(existing, 1);
                    if (sym->linkage == LINKAGE_INTERNAL) {
                        LLVMSetLinkage(existing, LLVMInternalLinkage);
                    }
                    if (sym->type.isConst) {
                        LLVMSetUnnamedAddr(existing, LLVMGlobalUnnamedAddr);
                    }
                }
            }
        }
    }

    uint64_t szDummy = 0;
    uint32_t alignVal = 0;
    if (cg_size_align_for_type(ctx, &sym->type, varType, &szDummy, &alignVal) && alignVal > 0) {
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
    LLVMValueRef fn = ensureFunction(ctx, sym->name, &sym->type, flattenedCount, paramTypes, sym);
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
        bool isBuiltinConst = (strcmp(name, "NULL") == 0 ||
                               strcmp(name, "true") == 0 ||
                               strcmp(name, "false") == 0);
        if (isBuiltinConst) {
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

        bool isBuiltinConst = (strcmp(name, "NULL") == 0 ||
                               strcmp(name, "true") == 0 ||
                               strcmp(name, "false") == 0);
        LLVMValueRef global = LLVMAddGlobal(ctx->module, varType, name);
        if (isBuiltinConst) {
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
