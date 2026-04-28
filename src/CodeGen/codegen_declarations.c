// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include "codegen_types.h"
#include "Syntax/symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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


static DesignatedInit* cg_find_initializer_for_symbol(const Symbol* sym) {
    return sym ? sym->initializer : NULL;
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
    bool useVariadicSRet = cg_should_lower_indirect_aggregate_return(ctx, returnLLVM);
    LLVMTypeRef fnReturnType = useVariadicSRet
        ? LLVMVoidTypeInContext(ctx->llvmContext)
        : returnLLVM;
    LLVMTypeRef* fnParamTypes = paramTypes;
    size_t fnParamCount = paramCount;
    LLVMTypeRef* loweredParamTypes = NULL;
    if (useVariadicSRet) {
        loweredParamTypes = (LLVMTypeRef*)calloc(paramCount + 1u, sizeof(LLVMTypeRef));
        if (!loweredParamTypes) {
            return NULL;
        }
        loweredParamTypes[0] = LLVMPointerType(returnLLVM, 0);
        for (size_t i = 0; i < paramCount; ++i) {
            loweredParamTypes[i + 1u] = paramTypes ? paramTypes[i] : NULL;
        }
        fnParamTypes = loweredParamTypes;
        fnParamCount = paramCount + 1u;
    }

    LLVMTypeRef fnType = LLVMFunctionType(fnReturnType,
                                          fnParamTypes,
                                          (unsigned)fnParamCount,
                                          isVariadic ? 1 : 0);
    free(loweredParamTypes);
    LLVMValueRef existing = LLVMGetNamedFunction(ctx->module, name);
    LLVMValueRef fn = existing ? existing : LLVMAddFunction(ctx->module, name, fnType);
    if (useVariadicSRet) {
        unsigned sretKind = LLVMGetEnumAttributeKindForName("sret", 4);
        if (sretKind != 0) {
            LLVMAttributeRef sretAttr = LLVMCreateTypeAttribute(ctx->llvmContext, sretKind, returnLLVM);
            LLVMAddAttributeAtIndex(fn, 1, sretAttr);
        }
    }

    if (symHint) {
        int callConv = (int)symHint->signature.callConv;
        if (callConv == 1) {
            if (supportsStdcall) {
                LLVMSetFunctionCallConv(fn, LLVMX86StdcallCallConv);
            }
        } else if (callConv == 2) {
            if (supportsFastcall) {
                LLVMSetFunctionCallConv(fn, LLVMX86FastcallCallConv);
            }
        } else if (callConv == 3) {
            LLVMSetFunctionCallConv(fn, LLVMCCallConv);
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
        /* Match the host clang toolchain used across CodeWork apps: file-scope
         * tentative definitions are emitted as common symbols so header-level
         * `int x;` declarations can coalesce with one strong definition or
         * other tentative declarations across translation units. */
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
    profiler_record_value("codegen_count_predeclare_symbol", 1);
    switch (sym->kind) {
        case SYMBOL_VARIABLE:
            profiler_record_value("codegen_count_predeclare_variable", 1);
            declareGlobalVariableSymbol(ctx, sym);
            break;
        case SYMBOL_FUNCTION:
            if (!sym->hasDefinition) {
                profiler_record_value("codegen_count_predeclare_function_decl_only", 1);
                break;
            }
            profiler_record_value("codegen_count_predeclare_function_with_definition", 1);
            declareFunctionSymbol(ctx, sym);
            break;
        case SYMBOL_STRUCT:
            profiler_record_value("codegen_count_predeclare_struct", 1);
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
    ProfilerScope scope = profiler_begin("codegen_predeclare_globals");
    if (!ctx) {
        profiler_end(scope);
        return;
    }

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
        profiler_end(scope);
        return;
    }

    if (!program || program->type != AST_PROGRAM) {
        profiler_end(scope);
        return;
    }

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
    profiler_end(scope);
}
