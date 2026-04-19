// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include "codegen_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    return type;
}

static bool cg_parsed_type_is_pointer_like(const ParsedType* type) {
    if (!type) return false;
    if (type->isFunctionPointer || type->pointerDepth > 0) return true;
    return parsedTypeCountDerivationsOfKind(type, TYPE_DERIVATION_POINTER) > 0;
}

static bool cg_try_eval_initializer_index(ASTNode* expr, unsigned long long* outIndex) {
    if (!expr || !outIndex) return false;
    switch (expr->type) {
        case AST_NUMBER_LITERAL:
            *outIndex = strtoull(expr->valueNode.value ? expr->valueNode.value : "0", NULL, 0);
            return true;
        case AST_CHAR_LITERAL:
            if (expr->valueNode.value && expr->valueNode.value[0] != '\0') {
                *outIndex = (unsigned char)expr->valueNode.value[0];
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

static unsigned long long cg_guess_compound_literal_length(ASTNode* literal) {
    if (!literal || literal->type != AST_COMPOUND_LITERAL) return 0;
    unsigned long long maxIndexPlusOne = 0;
    unsigned long long implicit = 0;
    for (size_t i = 0; i < literal->compoundLiteral.entryCount; ++i) {
        DesignatedInit* entry = literal->compoundLiteral.entries[i];
        if (!entry) continue;

        unsigned long long idx = implicit;
        if (entry->indexExpr) {
            if (!cg_try_eval_initializer_index(entry->indexExpr, &idx)) {
                return 0;
            }
            implicit = idx + 1;
        } else {
            implicit = idx + 1;
        }
        if (idx + 1 > maxIndexPlusOne) {
            maxIndexPlusOne = idx + 1;
        }
    }
    if (maxIndexPlusOne == 0) {
        return literal->compoundLiteral.entryCount;
    }
    return maxIndexPlusOne;
}

static bool cg_init_pointer_from_compound_literal(CodegenContext* ctx,
                                                  LLVMValueRef storage,
                                                  LLVMTypeRef storageType,
                                                  const ParsedType* varParsed,
                                                  ASTNode* literal) {
    if (!ctx || !storage || !literal || literal->type != AST_COMPOUND_LITERAL) return false;
    if (!cg_parsed_type_is_pointer_like(varParsed)) return false;

    const ParsedType* litParsed = &literal->compoundLiteral.literalType;
    LLVMTypeRef litType = cg_type_from_parsed(ctx, litParsed);
    const TypeDerivation* firstDeriv = parsedTypeGetDerivation(litParsed, 0);
    if (firstDeriv &&
        firstDeriv->kind == TYPE_DERIVATION_ARRAY &&
        !firstDeriv->as.array.isVLA &&
        (!firstDeriv->as.array.hasConstantSize || firstDeriv->as.array.constantSize <= 0)) {
        ParsedType elementParsed = parsedTypeArrayElementType(litParsed);
        LLVMTypeRef elementLLVM = cg_type_from_parsed(ctx, &elementParsed);
        parsedTypeFree(&elementParsed);
        if (!elementLLVM) {
            elementLLVM = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        unsigned long long guessedLen = cg_guess_compound_literal_length(literal);
        if (guessedLen == 0) {
            guessedLen = literal->compoundLiteral.entryCount;
        }
        if (guessedLen == 0) {
            guessedLen = 1;
        }
        litType = LLVMArrayType(elementLLVM, (unsigned)guessedLen);
        TypeDerivation* mutableArray =
            parsedTypeGetMutableArrayDerivation((ParsedType*)litParsed, 0);
        if (mutableArray) {
            mutableArray->as.array.hasConstantSize = true;
            mutableArray->as.array.constantSize = (long long)guessedLen;
        }
    }

    if (!litType || LLVMGetTypeKind(litType) == LLVMVoidTypeKind) {
        return false;
    }

    LLVMValueRef tmp = cg_build_entry_alloca(ctx, litType, "compound.literal.tmp");
    if (!cg_store_compound_literal_into_ptr(ctx, tmp, litType, litParsed, literal)) {
        return false;
    }

    LLVMValueRef ptrValue = tmp;
    if (LLVMGetTypeKind(litType) == LLVMArrayTypeKind) {
        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef idx[2] = { zero, zero };
        ptrValue = LLVMBuildGEP2(ctx->builder, litType, tmp, idx, 2, "compound.literal.decay");
    }

    LLVMValueRef casted = cg_cast_value(ctx,
                                        ptrValue,
                                        storageType,
                                        litParsed,
                                        varParsed,
                                        "compound.literal.ptr");
    if (!casted) {
        return false;
    }
    LLVMBuildStore(ctx->builder, casted, storage);
    return true;
}

static LLVMValueRef ensureIntegerLLVMValue(CodegenContext* ctx, LLVMValueRef value) {
    if (!value) return NULL;
    LLVMTypeRef ty = LLVMTypeOf(value);
    if (ty && LLVMGetTypeKind(ty) == LLVMIntegerTypeKind) {
        return value;
    }
    if (ty && LLVMGetTypeKind(ty) == LLVMPointerTypeKind) {
        LLVMTypeRef target = LLVMIntPtrTypeInContext(ctx->llvmContext, 0);
        return LLVMBuildPtrToInt(ctx->builder, value, target, "vla.ptrtoint");
    }
    return LLVMBuildIntCast(ctx->builder,
                            value,
                            LLVMInt32TypeInContext(ctx->llvmContext),
                            "vla.intcast");
}

static LLVMValueRef codegenVLAElementCount(CodegenContext* ctx, const ParsedType* type) {
    if (!ctx || !type) return NULL;
    LLVMValueRef total = NULL;
    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);

    for (size_t i = 0; i < type->derivationCount; ++i) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(type, i);
        if (!deriv || deriv->kind != TYPE_DERIVATION_ARRAY) {
            continue;
        }

        LLVMValueRef dimValue = NULL;
        if (!deriv->as.array.isVLA && deriv->as.array.hasConstantSize && deriv->as.array.constantSize > 0) {
            dimValue = LLVMConstInt(intptrTy, (unsigned long long)deriv->as.array.constantSize, 0);
        } else if (deriv->as.array.sizeExpr) {
            LLVMValueRef evaluated = codegenNode(ctx, deriv->as.array.sizeExpr);
            dimValue = ensureIntegerLLVMValue(ctx, evaluated);
        } else {
            dimValue = LLVMConstInt(intptrTy, 1, 0);
        }
        if (!dimValue) {
            return NULL;
        }
        if (!total) {
            total = dimValue;
        } else {
            total = LLVMBuildMul(ctx->builder, total, dimValue, "vla.total");
        }
    }
    return total;
}

LLVMValueRef codegenVariableDeclaration(CodegenContext* ctx, ASTNode* node) {
    ProfilerScope scope = profiler_begin("codegen_variable_declaration");
    profiler_record_value("codegen_count_variable_declaration", 1);
    if (node->type != AST_VARIABLE_DECLARATION) {
        fprintf(stderr, "Error: Invalid node type for codegenVariableDeclaration\n");
        profiler_end(scope);
        return NULL;
    }

    CG_DEBUG("[CG] Var decl count=%zu\n", node->varDecl.varCount);
    if (!ctx->currentScope || ctx->currentScope->parent == NULL) {
        profiler_end(scope);
        return NULL;
    }
    for (size_t i = 0; i < node->varDecl.varCount; i++) {
        ASTNode* varNameNode = node->varDecl.varNames[i];
        const ParsedType* varParsed = astVarDeclTypeAt(node, i);
        const ParsedType* effectiveParsed = varParsed ? varParsed : &node->varDecl.declaredType;
        const ParsedType* resolvedParsed = cg_resolve_typedef_parsed(ctx, effectiveParsed);
        const ParsedType* arrayParsed = (resolvedParsed && parsedTypeIsDirectArray(resolvedParsed))
            ? resolvedParsed
            : effectiveParsed;
        bool isArray = arrayParsed ? parsedTypeIsDirectArray(arrayParsed) : false;
        bool hasVLA = arrayParsed ? parsedTypeHasVLA(arrayParsed) : false;
        bool isStaticStorage = effectiveParsed && effectiveParsed->isStatic;

        if (varParsed && varParsed->inlineStructOrUnionDef) {
            codegenStructDefinition(ctx, varParsed->inlineStructOrUnionDef);
        }

        LLVMValueRef storage = NULL;
        LLVMTypeRef storageType = NULL;
        LLVMTypeRef elementLLVM = NULL;

        if (isStaticStorage) {
            const char* varName = (varNameNode && varNameNode->type == AST_IDENTIFIER)
                ? varNameNode->valueNode.value
                : "static";
            char globalName[256];
            static unsigned staticCounter = 0;
            const char* fnName = ctx->currentFunctionName ? ctx->currentFunctionName : "anon";
            snprintf(globalName, sizeof(globalName), ".static.%s.%s.%u", fnName, varName, staticCounter++);

            LLVMTypeRef varType = cg_type_from_parsed(ctx, effectiveParsed);
            if (!varType || LLVMGetTypeKind(varType) == LLVMVoidTypeKind) {
                varType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            LLVMValueRef global = LLVMAddGlobal(ctx->module, varType, globalName);
            LLVMSetLinkage(global, LLVMInternalLinkage);
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
            const ParsedType* storedParsed = resolvedParsed ? resolvedParsed : effectiveParsed;
            cg_scope_insert(ctx->currentScope,
                            varName,
                            global,
                            varType,
                            true,
                            isArray,
                            elementLLVM,
                            storedParsed);

            uint64_t szDummy = 0;
            uint32_t alignVal = 0;
            if (cg_size_align_for_type(ctx, storedParsed, varType, &szDummy, &alignVal) && alignVal > 0) {
                LLVMSetAlignment(global, alignVal);
            }

            DesignatedInit* init = node->varDecl.initializers ? node->varDecl.initializers[i] : NULL;
            if (init && init->expression) {
                LLVMValueRef constInit = cg_build_const_initializer(ctx,
                                                                    init->expression,
                                                                    varType,
                                                                    storedParsed);
                if (constInit) {
                    LLVMSetInitializer(global, constInit);
                    if (storedParsed && storedParsed->isConst) {
                        LLVMSetUnnamedAddr(global, LLVMGlobalUnnamedAddr);
                    }
                } else {
                    fprintf(stderr, "Error: static local initializer is not a constant expression\n");
                    return false;
                }
            }
            continue;
        }

        if (isArray) {
            LLVMValueRef vlaElementCount = NULL;
            if (hasVLA) {
                vlaElementCount = codegenVLAElementCount(ctx, arrayParsed);
                if (!vlaElementCount) {
                    fprintf(stderr, "Error: Failed to compute VLA length for '%s'\n", varNameNode->valueNode.value);
                    continue;
                }
                ParsedType base = parsedTypeClone(arrayParsed);
                while (parsedTypeIsDirectArray(&base)) {
                    ParsedType next = parsedTypeArrayElementType(&base);
                    parsedTypeFree(&base);
                    base = next;
                }
                elementLLVM = cg_type_from_parsed(ctx, &base);
                parsedTypeFree(&base);
                if (!elementLLVM || LLVMGetTypeKind(elementLLVM) == LLVMVoidTypeKind) {
                    elementLLVM = LLVMInt32TypeInContext(ctx->llvmContext);
                }
                storage = LLVMBuildArrayAlloca(ctx->builder,
                                               elementLLVM,
                                               vlaElementCount,
                                               varNameNode->valueNode.value);
                storageType = LLVMTypeOf(storage);
            } else {
                storageType = cg_type_from_parsed(ctx, effectiveParsed);
                ParsedType element = parsedTypeArrayElementType(arrayParsed);
                elementLLVM = cg_type_from_parsed(ctx, &element);
                parsedTypeFree(&element);
                if (!elementLLVM || LLVMGetTypeKind(elementLLVM) == LLVMVoidTypeKind) {
                    elementLLVM = LLVMInt32TypeInContext(ctx->llvmContext);
                }
                if (!storageType || LLVMGetTypeKind(storageType) == LLVMVoidTypeKind) {
                    storageType = LLVMArrayType(elementLLVM, 1);
                }
                storage = cg_build_entry_alloca(ctx, storageType, varNameNode->valueNode.value);
            }
            const ParsedType* storedParsed = arrayParsed ? arrayParsed : effectiveParsed;
            cg_scope_insert(ctx->currentScope,
                            varNameNode->valueNode.value,
                            storage,
                            storageType,
                            false,
                            true,
                            elementLLVM,
                            storedParsed);
            if (getenv("DEBUG_PTR_ARITH")) {
                fprintf(stderr,
                        "[ptr-arith] var '%s' storedParsed kind=%d ptrDepth=%d derivs=%zu isVLA=%d\n",
                        varNameNode && varNameNode->valueNode.value ? varNameNode->valueNode.value : "<anon>",
                        storedParsed ? storedParsed->kind : -1,
                        storedParsed ? storedParsed->pointerDepth : -1,
                        storedParsed ? storedParsed->derivationCount : 0,
                        (storedParsed && parsedTypeHasVLA(storedParsed)) ? 1 : 0);
            }
            if (hasVLA) {
                NamedValue* entry = cg_scope_lookup(ctx->currentScope, varNameNode->valueNode.value);
                if (entry) {
                    entry->vlaElementCount = vlaElementCount;
                }
            }
        } else {
            LLVMTypeRef varType = cg_type_from_parsed(ctx, effectiveParsed);
            if (!varType || LLVMGetTypeKind(varType) == LLVMVoidTypeKind) {
                varType = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
            }
            storage = cg_build_entry_alloca(ctx, varType, varNameNode->valueNode.value);
            storageType = varType;
            const ParsedType* storedParsed = resolvedParsed ? resolvedParsed : effectiveParsed;
            LLVMTypeRef elementLLVM = cg_element_type_from_pointer_parsed(ctx, storedParsed);
            cg_scope_insert(ctx->currentScope,
                            varNameNode->valueNode.value,
                            storage,
                            varType,
                            false,
                            false,
                            elementLLVM,
                            storedParsed);
            if (getenv("DEBUG_PTR_ARITH")) {
                char* vt = LLVMPrintTypeToString(varType);
                fprintf(stderr,
                        "[ptr-arith] var '%s' scalar storedParsed kind=%d ptrDepth=%d derivs=%zu varType=%s\n",
                        varNameNode && varNameNode->valueNode.value ? varNameNode->valueNode.value : "<anon>",
                        storedParsed ? storedParsed->kind : -1,
                        storedParsed ? storedParsed->pointerDepth : -1,
                        storedParsed ? storedParsed->derivationCount : 0,
                        vt ? vt : "<null>");
                if (vt) LLVMDisposeMessage(vt);
            }
        }
        const ParsedType* alignParsed = varParsed ? varParsed : &node->varDecl.declaredType;
        uint64_t szDummy = 0;
        uint32_t alignVal = 0;
        if (cg_size_align_for_type(ctx, alignParsed, storageType, &szDummy, &alignVal) && alignVal > 0) {
            LLVMSetAlignment(storage, alignVal);
        }

        DesignatedInit* init = node->varDecl.initializers ? node->varDecl.initializers[i] : NULL;
        if (init && init->expression) {
            LLVMTypeRef valueType = LLVMGetAllocatedType(storage);
            if (!valueType) {
                valueType = storageType;
            }
            if (init->expression->type == AST_COMPOUND_LITERAL) {
                if (!isArray && cg_init_pointer_from_compound_literal(ctx,
                                                                      storage,
                                                                      valueType,
                                                                      effectiveParsed,
                                                                      init->expression)) {
                    continue;
                }
                if (!cg_store_compound_literal_into_ptr(ctx,
                                                        storage,
                                                        valueType,
                                                        isArray ? arrayParsed : effectiveParsed,
                                                        init->expression)) {
                    fprintf(stderr, "Error: Failed to emit initializer for variable\n");
                }
            } else {
                if (!isArray) {
                    if (!cg_store_initializer_expression(ctx,
                                                         storage,
                                                         valueType,
                                                         effectiveParsed,
                                                         init->expression)) {
                        fprintf(stderr, "Error: Failed to emit initializer for variable\n");
                    }
                } else {
                    if (!hasVLA &&
                        valueType &&
                        LLVMGetTypeKind(valueType) == LLVMArrayTypeKind &&
                        init->expression->type == AST_STRING_LITERAL) {
                        LLVMValueRef constArray = cg_build_const_initializer(ctx,
                                                                             init->expression,
                                                                             valueType,
                                                                             arrayParsed);
                        if (constArray) {
                            LLVMBuildStore(ctx->builder, constArray, storage);
                        } else if (!cg_store_initializer_expression(ctx,
                                                                    storage,
                                                                    valueType,
                                                                    arrayParsed,
                                                                    init->expression)) {
                            fprintf(stderr, "Error: Failed to emit initializer for array variable\n");
                        }
                    } else if (!cg_store_initializer_expression(ctx,
                                                                storage,
                                                                valueType,
                                                                arrayParsed,
                                                                init->expression)) {
                        fprintf(stderr, "Error: Failed to emit initializer for array variable\n");
                    }
                }
            }
        }
    }

    profiler_end(scope);
    return NULL;
}
