#include "codegen_private.h"

#include "Syntax/const_eval.h"
#include "codegen_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_const_string_counter = 0;

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

static LLVMValueRef cg_build_const_initializer(CodegenContext* ctx,
                                               ASTNode* expr,
                                               LLVMTypeRef targetType,
                                               const ParsedType* parsedType) {
    if (!ctx || !expr || !targetType) return NULL;

    LLVMTypeKind targetKind = LLVMGetTypeKind(targetType);
    if (targetKind == LLVMVoidTypeKind) {
        return NULL;
    }
    if (expr->type == AST_STRING_LITERAL) {
        const char* payload = NULL;
        ast_literal_encoding(expr->valueNode.value, &payload);
        const char* text = payload ? payload : expr->valueNode.value;
        if (targetKind == LLVMPointerTypeKind) {
            return cg_make_const_string_global(ctx, text, targetType);
        }
        if (targetKind == LLVMArrayTypeKind) {
            LLVMTypeRef element = LLVMGetElementType(targetType);
            if (element && LLVMGetTypeKind(element) == LLVMIntegerTypeKind &&
                LLVMGetIntTypeWidth(element) == 8) {
                size_t len = text ? strlen(text) : 0;
                LLVMValueRef strConst =
                    LLVMConstStringInContext(ctx->llvmContext, text ? text : "", (unsigned)len, 0);
                return strConst;
            }
        }
        return NULL;
    }

    struct Scope* globalScope = NULL;
    if (ctx->semanticModel) {
        globalScope = semanticModelGetGlobalScope(ctx->semanticModel);
    }
    ConstEvalResult res = constEval(expr, globalScope, true);
    if (!res.isConst) {
        return NULL;
    }
    return cg_const_from_eval(ctx, &res, targetType, parsedType);
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
                            LLVMTypeRef* paramTypes) {
    if (!ctx || !name) return NULL;

    LLVMTypeRef returnLLVM = returnType ? cg_type_from_parsed(ctx, returnType) : LLVMVoidTypeInContext(ctx->llvmContext);
    if (!returnLLVM || LLVMGetTypeKind(returnLLVM) == LLVMVoidTypeKind) {
        returnLLVM = LLVMVoidTypeInContext(ctx->llvmContext);
    }

    LLVMTypeRef fnType = LLVMFunctionType(returnLLVM, paramTypes, (unsigned)paramCount, 0);
    LLVMValueRef existing = LLVMGetNamedFunction(ctx->module, name);
    if (existing) {
        return existing;
    }
    LLVMValueRef fn = LLVMAddFunction(ctx->module, name, fnType);
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
    LLVMValueRef fn = ensureFunction(ctx, name, returnType, flattenedCount, paramTypes);
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
        if (init && init->expression && !parsedTypeHasVLA(&sym->type)) {
            LLVMTypeRef elementType = LLVMGlobalGetValueType(existing);
            if (elementType) {
                LLVMValueRef constInit = cg_build_const_initializer(ctx,
                                                                    init->expression,
                                                                    elementType,
                                                                    &sym->type);
                if (constInit) {
                    LLVMSetInitializer(existing, constInit);
                }
            }
        }
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
    LLVMValueRef fn = ensureFunction(ctx, sym->name, &sym->type, flattenedCount, paramTypes);
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
        LLVMTypeRef varType = cg_type_from_parsed(ctx, parsedType);
        if (!varType || LLVMGetTypeKind(varType) == LLVMVoidTypeKind) {
            varType = LLVMInt32TypeInContext(ctx->llvmContext);
        }

        bool isArray = parsedType ? parsedTypeIsDirectArray(parsedType) : false;
        LLVMValueRef existing = LLVMGetNamedGlobal(ctx->module, name);
        if (existing) {
            LLVMTypeRef existingType = LLVMGetElementType(LLVMTypeOf(existing));
            if (!existingType || LLVMGetTypeKind(existingType) == LLVMVoidTypeKind) {
                existingType = varType;
            }
            LLVMTypeRef elementLLVM = NULL;
            if (isArray) {
                ParsedType element = parsedTypeArrayElementType(parsedType ? parsedType : &node->varDecl.declaredType);
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
            ParsedType element = parsedTypeArrayElementType(parsedType ? parsedType : &node->varDecl.declaredType);
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
