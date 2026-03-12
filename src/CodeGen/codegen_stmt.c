#include "codegen_private.h"

#include "codegen_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static LLVMValueRef ensureIntegerLLVMValue(CodegenContext* ctx, LLVMValueRef value);
static LLVMValueRef codegenVLAElementCount(CodegenContext* ctx, const ParsedType* type);

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

static bool cg_parsed_type_is_pointer_like(const ParsedType* type) {
    if (!type) return false;
    if (type->isFunctionPointer || type->pointerDepth > 0) return true;
    return parsedTypeCountDerivationsOfKind(type, TYPE_DERIVATION_POINTER) > 0;
}

static bool cg_parsed_type_is_direct_function(const ParsedType* type) {
    return type &&
           type->derivationCount > 0 &&
           type->derivations &&
           type->derivations[0].kind == TYPE_DERIVATION_FUNCTION;
}

static void cg_adjust_parameter_type(ParsedType* type) {
    if (!type) return;
    parsedTypeAdjustArrayParameter(type);
    if (!cg_parsed_type_is_direct_function(type)) {
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
                return 0; // unknown size
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
            guessedLen = 1; // fallback to avoid zero-length LLVM array
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

    LLVMValueRef tmp = LLVMBuildAlloca(ctx->builder, litType, "compound.literal.tmp");
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

static bool cg_node_is_expression_type(ASTNodeType type) {
    switch (type) {
        case AST_ASSIGNMENT:
        case AST_BINARY_EXPRESSION:
        case AST_UNARY_EXPRESSION:
        case AST_TERNARY_EXPRESSION:
        case AST_COMMA_EXPRESSION:
        case AST_CAST_EXPRESSION:
        case AST_COMPOUND_LITERAL:
        case AST_ARRAY_ACCESS:
        case AST_POINTER_ACCESS:
        case AST_POINTER_DEREFERENCE:
        case AST_FUNCTION_CALL:
        case AST_IDENTIFIER:
        case AST_NUMBER_LITERAL:
        case AST_CHAR_LITERAL:
        case AST_STRING_LITERAL:
        case AST_SIZEOF:
        case AST_STATEMENT_EXPRESSION:
            return true;
        default:
            return false;
    }
}

static LLVMValueRef cg_finalize_statement_expr_result(CodegenContext* ctx, LLVMValueRef value) {
    if (!value) {
        return NULL;
    }
    LLVMTypeRef valueType = LLVMTypeOf(value);
    if (!valueType || LLVMGetTypeKind(valueType) == LLVMVoidTypeKind) {
        return value;
    }

    if (!LLVMIsConstant(value)) {
        LLVMSetValueName2(value, "stmt.expr.result", strlen("stmt.expr.result"));
        return value;
    }

    LLVMValueRef slot = LLVMBuildAlloca(ctx->builder, valueType, "stmt.expr.result.slot");
    LLVMBuildStore(ctx->builder, value, slot);
    return LLVMBuildLoad2(ctx->builder, valueType, slot, "stmt.expr.result");
}

LLVMValueRef codegenProgram(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_PROGRAM) {
        fprintf(stderr, "Error: Invalid node type for codegenProgram\n");
        return NULL;
    }

    CG_DEBUG("[CG] Program statements=%zu\n", node->block.statementCount);
    LLVMValueRef last = NULL;
    for (size_t i = 0; i < node->block.statementCount; i++) {
        last = codegenNode(ctx, node->block.statements[i]);
    }
    return last;
}


LLVMValueRef codegenBlock(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_BLOCK) {
        fprintf(stderr, "Error: Invalid node type for codegenBlock\n");
        return NULL;
    }

    CG_DEBUG("[CG] Block statements=%zu\n", node->block.statementCount);
    cg_scope_push(ctx);
    LLVMValueRef last = NULL;
    for (size_t i = 0; i < node->block.statementCount; i++) {
        last = codegenNode(ctx, node->block.statements[i]);
    }
    cg_scope_pop(ctx);
    return last;
}

LLVMValueRef codegenStatementExpression(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_STATEMENT_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenStatementExpression\n");
        return NULL;
    }
    ASTNode* block = node->statementExpr.block;
    if (!block) {
        fprintf(stderr, "Error: Statement expression missing body\n");
        return NULL;
    }

    if (!cg_scope_push(ctx)) {
        fprintf(stderr, "Error: Failed to create scope for statement expression\n");
        return NULL;
    }

    LLVMValueRef resultValue = NULL;
    if (block->type == AST_BLOCK) {
        size_t count = block->block.statementCount;
        for (size_t i = 0; i < count; ++i) {
            ASTNode* stmt = block->block.statements[i];
            if (!stmt) {
                continue;
            }
            bool isLast = (i + 1 == count);
            if (isLast && cg_node_is_expression_type(stmt->type)) {
                resultValue = codegenNode(ctx, stmt);
            } else {
                codegenNode(ctx, stmt);
            }
        }
    } else if (cg_node_is_expression_type(block->type)) {
        resultValue = codegenNode(ctx, block);
    } else {
        codegenNode(ctx, block);
    }

    cg_scope_pop(ctx);

    if (!resultValue) {
        fprintf(stderr, "Error: Statement expression did not yield a value\n");
        return NULL;
    }
    return cg_finalize_statement_expr_result(ctx, resultValue);
}
LLVMValueRef codegenVariableDeclaration(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_VARIABLE_DECLARATION) {
        fprintf(stderr, "Error: Invalid node type for codegenVariableDeclaration\n");
        return NULL;
    }

    CG_DEBUG("[CG] Var decl count=%zu\n", node->varDecl.varCount);
    if (!ctx->currentScope || ctx->currentScope->parent == NULL) {
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

        // Inline struct/union definitions need layouts before emitting storage/initializers
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
            const ParsedType* storedParsed = effectiveParsed;
            if (storedParsed && storedParsed->kind == TYPE_NAMED) {
                CGTypeCache* cache = cg_context_get_type_cache(ctx);
                CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, storedParsed->userTypeName);
                if (info) {
                    storedParsed = &info->parsedType;
                }
            }
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
                storage = LLVMBuildAlloca(ctx->builder, storageType, varNameNode->valueNode.value);
            }
            const ParsedType* storedParsed = varParsed ? varParsed : &node->varDecl.declaredType;
            if (storedParsed && storedParsed->kind == TYPE_NAMED) {
                CGTypeCache* cache = cg_context_get_type_cache(ctx);
                CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, storedParsed->userTypeName);
                if (info) {
                    storedParsed = &info->parsedType;
                }
            }
            cg_scope_insert(ctx->currentScope,
                            varNameNode->valueNode.value,
                            storage,
                            storageType,
                            false,
                            true,
                            elementLLVM,
                            storedParsed);
            if (hasVLA) {
                NamedValue* entry = cg_scope_lookup(ctx->currentScope, varNameNode->valueNode.value);
                if (entry) {
                    entry->vlaElementCount = vlaElementCount;
                }
            }
        } else {
            LLVMTypeRef varType = cg_type_from_parsed(ctx, varParsed);
            if (!varType || LLVMGetTypeKind(varType) == LLVMVoidTypeKind) {
                varType = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
            }
            storage = LLVMBuildAlloca(ctx->builder, varType, varNameNode->valueNode.value);
            storageType = varType;
            const ParsedType* storedParsed = varParsed ? varParsed : &node->varDecl.declaredType;
            if (storedParsed && storedParsed->kind == TYPE_NAMED) {
                CGTypeCache* cache = cg_context_get_type_cache(ctx);
                CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, storedParsed->userTypeName);
                if (info) {
                    storedParsed = &info->parsedType;
                }
            }
            LLVMTypeRef elementLLVM = cg_element_type_from_pointer_parsed(ctx, storedParsed);
            cg_scope_insert(ctx->currentScope,
                            varNameNode->valueNode.value,
                            storage,
                            varType,
                            false,
                            false,
                            elementLLVM,
                            storedParsed);
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
                    LLVMValueRef initValue = codegenNode(ctx, init->expression);
                    if (initValue) {
                        LLVMValueRef casted = cg_cast_value(ctx,
                                                            initValue,
                                                            valueType,
                                                            NULL,
                                                            effectiveParsed,
                                                            "init.store");
                        cg_build_store(ctx, casted, storage, effectiveParsed);
                    }
                } else {
                    if (!cg_store_initializer_expression(ctx,
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

    return NULL;
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

LLVMValueRef codegenIfStatement(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_IF_STATEMENT) {
        fprintf(stderr, "Error: Invalid node type for codegenIfStatement\n");
        return NULL;
    }

    LLVMValueRef cond = codegenNode(ctx, node->ifStmt.condition);
    if (!cond) {
        fprintf(stderr, "Error: Failed to generate condition for if statement\n");
        return NULL;
    }

    cond = cg_build_truthy(ctx, cond, NULL, "ifcond");
    if (!cond) {
        fprintf(stderr, "Error: Failed to convert if condition to boolean\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef thenBB = LLVMAppendBasicBlock(func, "then");
    LLVMBasicBlockRef elseBB = node->ifStmt.elseBranch ? LLVMAppendBasicBlock(func, "else") : NULL;
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(func, "ifcont");

    LLVMBuildCondBr(ctx->builder, cond, thenBB, elseBB ? elseBB : mergeBB);

    LLVMPositionBuilderAtEnd(ctx->builder, thenBB);
    codegenNode(ctx, node->ifStmt.thenBranch);
    if (!LLVMGetInsertBlock(ctx->builder) || !LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder))) {
        LLVMBuildBr(ctx->builder, mergeBB);
    }

    if (elseBB) {
        LLVMPositionBuilderAtEnd(ctx->builder, elseBB);
        codegenNode(ctx, node->ifStmt.elseBranch);
        if (!LLVMGetInsertBlock(ctx->builder) || !LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder))) {
            LLVMBuildBr(ctx->builder, mergeBB);
        }
    }

    LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
    return NULL;
}


LLVMValueRef codegenWhileLoop(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_WHILE_LOOP) {
        fprintf(stderr, "Error: Invalid node type for codegenWhileLoop\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(func, "loopcond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(func, "loopbody");
    LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(func, "loopend");

    if (!node->whileLoop.isDoWhile) {
        LLVMBasicBlockRef current = LLVMGetInsertBlock(ctx->builder);
        if (current && !LLVMGetBasicBlockTerminator(current)) {
            LLVMBuildBr(ctx->builder, condBB);
        }

        LLVMPositionBuilderAtEnd(ctx->builder, condBB);
        LLVMValueRef cond = codegenNode(ctx, node->whileLoop.condition);
        cond = cg_build_truthy(ctx, cond, NULL, "loopcond");
        if (!cond) {
            fprintf(stderr, "Error: Failed to convert loop condition to boolean\n");
            return NULL;
        }
        LLVMBuildCondBr(ctx->builder, cond, bodyBB, afterBB);

        LLVMPositionBuilderAtEnd(ctx->builder, bodyBB);
        cg_loop_push(ctx, afterBB, condBB);
        codegenNode(ctx, node->whileLoop.body);
        cg_loop_pop(ctx);
        LLVMBasicBlockRef bodyEnd = LLVMGetInsertBlock(ctx->builder);
        if (bodyEnd && !LLVMGetBasicBlockTerminator(bodyEnd)) {
            LLVMBuildBr(ctx->builder, condBB);
        }
    } else {
        // do-while: execute body first, then evaluate the loop condition.
        LLVMBasicBlockRef current = LLVMGetInsertBlock(ctx->builder);
        if (current && !LLVMGetBasicBlockTerminator(current)) {
            LLVMBuildBr(ctx->builder, bodyBB);
        }

        LLVMPositionBuilderAtEnd(ctx->builder, bodyBB);
        cg_loop_push(ctx, afterBB, condBB);
        codegenNode(ctx, node->whileLoop.body);
        cg_loop_pop(ctx);
        LLVMBasicBlockRef bodyEnd = LLVMGetInsertBlock(ctx->builder);
        if (bodyEnd && !LLVMGetBasicBlockTerminator(bodyEnd)) {
            LLVMBuildBr(ctx->builder, condBB);
        }

        LLVMPositionBuilderAtEnd(ctx->builder, condBB);
        LLVMValueRef cond = codegenNode(ctx, node->whileLoop.condition);
        cond = cg_build_truthy(ctx, cond, NULL, "loopcond");
        if (!cond) {
            fprintf(stderr, "Error: Failed to convert do-while condition to boolean\n");
            return NULL;
        }
        LLVMBuildCondBr(ctx->builder, cond, bodyBB, afterBB);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, afterBB);
    return NULL;
}


LLVMValueRef codegenForLoop(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_FOR_LOOP) {
        fprintf(stderr, "Error: Invalid node type for codegenForLoop\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(func, "forcond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(func, "forbody");
    LLVMBasicBlockRef incBB = LLVMAppendBasicBlock(func, "forinc");
    LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(func, "forend");

    if (node->forLoop.initializer) {
        codegenNode(ctx, node->forLoop.initializer);
    }

    LLVMBuildBr(ctx->builder, condBB);

    LLVMPositionBuilderAtEnd(ctx->builder, condBB);
    LLVMValueRef cond = node->forLoop.condition ? codegenNode(ctx, node->forLoop.condition)
                                                : LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 1, 0);
    cond = cg_build_truthy(ctx, cond, NULL, "forcond");
    if (!cond) {
        fprintf(stderr, "Error: Failed to convert for condition to boolean\n");
        return NULL;
    }
    LLVMBuildCondBr(ctx->builder, cond, bodyBB, afterBB);

    LLVMPositionBuilderAtEnd(ctx->builder, bodyBB);
    cg_loop_push(ctx, afterBB, incBB);
    codegenNode(ctx, node->forLoop.body);
    cg_loop_pop(ctx);
    LLVMBuildBr(ctx->builder, incBB);

    LLVMPositionBuilderAtEnd(ctx->builder, incBB);
    if (node->forLoop.increment) {
        codegenNode(ctx, node->forLoop.increment);
    }
    LLVMBuildBr(ctx->builder, condBB);

    LLVMPositionBuilderAtEnd(ctx->builder, afterBB);
    return NULL;
}


LLVMValueRef codegenFunctionDefinition(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_FUNCTION_DEFINITION) {
        fprintf(stderr, "Error: Invalid node type for codegenFunctionDefinition\n");
        return NULL;
    }

    const char* dbgRun = getenv("DEBUG_RUN");
    const char* fnLabel = (node->functionDef.funcName && node->functionDef.funcName->valueNode.value)
        ? node->functionDef.funcName->valueNode.value
        : "<anon>";
    if (dbgRun) {
        fprintf(stderr, "[CG] enter function %s\n", fnLabel);
    }

    CG_DEBUG("[CG] Function definition paramCount=%zu\n", node->functionDef.paramCount);
    LLVMTypeRef returnType = cg_type_from_parsed(ctx, &node->functionDef.returnType);
    if (returnType && LLVMGetTypeKind(returnType) == LLVMFunctionTypeKind) {
        returnType = LLVMPointerType(returnType, 0);
    } else if (returnType && LLVMGetTypeKind(returnType) == LLVMArrayTypeKind) {
        returnType = LLVMPointerType(returnType, 0);
    }
    if (!returnType) {
        returnType = LLVMVoidTypeInContext(ctx->llvmContext);
    }

    CGParamInfo* paramInfos = NULL;
    size_t paramCount = cg_expand_parameters(node->functionDef.parameters,
                                             node->functionDef.paramCount,
                                             &paramInfos,
                                             NULL);
    ParsedType* adjustedParamTypes = NULL;
    LLVMTypeRef* paramTypes = NULL;
    if (paramCount > 0) {
        adjustedParamTypes = (ParsedType*)calloc(paramCount, sizeof(ParsedType));
        paramTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * paramCount);
        if (!paramTypes || !adjustedParamTypes) {
            if (adjustedParamTypes) {
                free(adjustedParamTypes);
            }
            if (paramTypes) {
                free(paramTypes);
            }
            cg_free_param_infos(paramInfos);
            return NULL;
        }
        for (size_t i = 0; i < paramCount; i++) {
            const ParsedType* paramType = paramInfos[i].parsedType;
            adjustedParamTypes[i] = parsedTypeClone(paramType);
            cg_adjust_parameter_type(&adjustedParamTypes[i]);
            LLVMTypeRef inferred = cg_type_from_parsed(ctx, &adjustedParamTypes[i]);
            if (!inferred || LLVMGetTypeKind(inferred) == LLVMVoidTypeKind) {
                inferred = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            paramTypes[i] = inferred;
        }
    }

    LLVMTypeRef funcType = LLVMFunctionType(returnType,
                                            paramTypes,
                                            (unsigned)paramCount,
                                            node->functionDef.isVariadic ? 1 : 0);
    const char* fnName = node->functionDef.funcName->valueNode.value;
    LLVMValueRef function = LLVMGetNamedFunction(ctx->module, fnName);
    if (function) {
        // If a prototype already exists, reuse it; otherwise fall back to a fresh function.
        // Best-effort type nudge: replace the function's type if it mismatches.
        LLVMTypeRef existingType = NULL;
        LLVMTypeRef valueType = LLVMTypeOf(function);
        if (valueType && LLVMGetTypeKind(valueType) == LLVMFunctionTypeKind) {
            existingType = valueType;
        } else {
            existingType = cg_dereference_ptr_type(ctx, valueType, "function redeclaration");
        }
        bool opaqueFallback = false;
        if (valueType && LLVMGetTypeKind(valueType) == LLVMPointerTypeKind) {
            opaqueFallback = true;
        }
        if (existingType &&
            LLVMGetTypeKind(existingType) == LLVMIntegerTypeKind &&
            LLVMGetIntTypeWidth(existingType) == 8) {
            opaqueFallback = true;
        }
        if (existingType != funcType && !opaqueFallback) {
            if (getenv("FISICS_DEBUG_CONST")) {
                char* valueStr = valueType ? LLVMPrintTypeToString(valueType) : NULL;
                char* existStr = existingType ? LLVMPrintTypeToString(existingType) : NULL;
                char* funcStr = LLVMPrintTypeToString(funcType);
                char* fnValStr = LLVMPrintValueToString(function);
                fprintf(stderr,
                        "[fn] deleting prototype for %s due to signature mismatch (value=%s, existing=%s, new=%s)\n",
                        fnName,
                        valueStr ? valueStr : "<null>",
                        existStr ? existStr : "<null>",
                        funcStr ? funcStr : "<null>");
                fprintf(stderr, "[fn] existing value: %s\n", fnValStr ? fnValStr : "<null>");
                if (valueStr) LLVMDisposeMessage(valueStr);
                if (existStr) LLVMDisposeMessage(existStr);
                if (funcStr) LLVMDisposeMessage(funcStr);
                if (fnValStr) LLVMDisposeMessage(fnValStr);
            }
            LLVMDeleteFunction(function);
            function = NULL;
        }
    }
    if (!function) {
        function = LLVMAddFunction(ctx->module, fnName, funcType);
    }
    LLVMSetLinkage(function, LLVMExternalLinkage);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entry);

    const ParsedType* previousReturnType = ctx->currentFunctionReturnType;
    ctx->currentFunctionReturnType = &node->functionDef.returnType;
    const char* previousFunctionName = ctx->currentFunctionName;
    ctx->currentFunctionName = fnName;

    cg_scope_push(ctx);

    for (size_t i = 0; i < paramCount; i++) {
        CGParamInfo* info = &paramInfos[i];
        ASTNode* nameNode = info->nameNode;
        const char* label =
            (nameNode && nameNode->type == AST_IDENTIFIER && nameNode->valueNode.value)
                ? nameNode->valueNode.value
            : "param";
        LLVMTypeRef paramType = paramTypes ? paramTypes[i] : LLVMInt32TypeInContext(ctx->llvmContext);
        LLVMValueRef allocaInst = LLVMBuildAlloca(ctx->builder, paramType, label);
        LLVMBuildStore(ctx->builder, LLVMGetParam(function, (unsigned)i), allocaInst);
        const ParsedType* storedType = adjustedParamTypes
            ? &adjustedParamTypes[i]
            : (info->parsedType ? info->parsedType : &info->declaration->varDecl.declaredType);
        LLVMTypeRef elementLLVM = cg_element_type_from_pointer_parsed(ctx, storedType);
        cg_scope_insert(ctx->currentScope,
                        label,
                        allocaInst,
                        paramType,
                        false,
                        elementLLVM != NULL /* addressOnly */ ? false : false,
                        elementLLVM,
                        storedType);
    }

    if (dbgRun) fprintf(stderr, "[CG] body begin %s\n", fnLabel);
    codegenNode(ctx, node->functionDef.body);
    if (dbgRun) fprintf(stderr, "[CG] body end %s\n", fnLabel);

    cg_scope_pop(ctx);
    ctx->currentFunctionReturnType = previousReturnType;
    ctx->currentFunctionName = previousFunctionName;
    cg_free_param_infos(paramInfos);
    free(paramTypes);
    if (adjustedParamTypes) {
        for (size_t i = 0; i < paramCount; ++i) {
            parsedTypeFree(&adjustedParamTypes[i]);
        }
        free(adjustedParamTypes);
    }
    if (ctx->verifyFunctions && function) {
        if (LLVMVerifyFunction(function, LLVMPrintMessageAction) != 0) {
            fprintf(stderr, "Warning: LLVM verification failed for function %s\n",
                    node->functionDef.funcName && node->functionDef.funcName->valueNode.value
                        ? node->functionDef.funcName->valueNode.value
                        : "<anonymous>");
        }
    }
    if (dbgRun) {
        fprintf(stderr, "[CG] exit function %s\n", fnLabel);
    }
    return function;
}


LLVMValueRef codegenTypedef(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_TYPEDEF) {
        fprintf(stderr, "Error: Invalid node type for codegenTypedef\n");
        return NULL;
    }

    CG_DEBUG("[CG] Typedef encountered\n");
    const char* aliasName = (node->typedefStmt.alias && node->typedefStmt.alias->type == AST_IDENTIFIER)
        ? node->typedefStmt.alias->valueNode.value
        : NULL;
    if (!aliasName || aliasName[0] == '\0') {
        return NULL;
    }

    if (cg_context_get_type_cache(ctx)) {
        return NULL; /* semantic model already contributed this typedef */
    }

    LLVMTypeRef aliased = cg_type_from_parsed(ctx, &node->typedefStmt.baseType);
    if (!aliased || LLVMGetTypeKind(aliased) == LLVMVoidTypeKind) {
        aliased = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    cg_context_cache_named_type(ctx, aliasName, aliased);
    return NULL;
}


LLVMValueRef codegenEnumDefinition(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_ENUM_DEFINITION) {
        fprintf(stderr, "Error: Invalid node type for codegenEnumDefinition\n");
        return NULL;
    }
    (void)ctx;
    /* Currently enums only contribute constants during semantics; IR emission is a no-op. */
    return NULL;
}


LLVMValueRef codegenReturn(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_RETURN) {
        fprintf(stderr, "Error: Invalid node type for codegenReturn\n");
        return NULL;
    }

    CG_DEBUG("[CG] Return begin\n");
    LLVMTypeRef declaredReturnLLVM = NULL;
    if (ctx->currentFunctionReturnType) {
        declaredReturnLLVM = cg_type_from_parsed(ctx, ctx->currentFunctionReturnType);
        if (declaredReturnLLVM && LLVMGetTypeKind(declaredReturnLLVM) == LLVMFunctionTypeKind) {
            declaredReturnLLVM = LLVMPointerType(declaredReturnLLVM, 0);
        } else if (declaredReturnLLVM && LLVMGetTypeKind(declaredReturnLLVM) == LLVMArrayTypeKind) {
            declaredReturnLLVM = LLVMPointerType(declaredReturnLLVM, 0);
        }
        if (!declaredReturnLLVM) {
            declaredReturnLLVM = LLVMVoidTypeInContext(ctx->llvmContext);
        }
    }

    if (node->returnStmt.returnValue) {
        LLVMValueRef value = codegenNode(ctx, node->returnStmt.returnValue);
        if (!value) {
            fprintf(stderr, "Error: Failed to generate return value\n");
            return NULL;
        }

        if (declaredReturnLLVM && LLVMGetTypeKind(declaredReturnLLVM) != LLVMVoidTypeKind) {
            value = cg_cast_value(ctx,
                                  value,
                                  declaredReturnLLVM,
                                  NULL,
                                  ctx->currentFunctionReturnType,
                                  "return.cast");
        }

        CG_DEBUG("[CG] Return emitting with value\n");
        return LLVMBuildRet(ctx->builder, value);
    }

    if (declaredReturnLLVM && LLVMGetTypeKind(declaredReturnLLVM) != LLVMVoidTypeKind) {
        fprintf(stderr, "Error: Non-void function missing return value; defaulting to zero\n");
        LLVMValueRef zero = LLVMConstNull(declaredReturnLLVM);
        return LLVMBuildRet(ctx->builder, zero);
    }

    CG_DEBUG("[CG] Return emitting void\n");
    return LLVMBuildRetVoid(ctx->builder);
}


LLVMValueRef codegenBreak(CodegenContext* ctx, ASTNode* node) {
    (void)node;
    LoopTarget target = cg_loop_peek(ctx);
    if (!target.breakBB) {
        fprintf(stderr, "Error: 'break' used outside of loop\n");
        return NULL;
    }
    return LLVMBuildBr(ctx->builder, target.breakBB);
}


LLVMValueRef codegenContinue(CodegenContext* ctx, ASTNode* node) {
    (void)node;
    LoopTarget target = cg_loop_peek_for_continue(ctx);
    if (!target.continueBB) {
        fprintf(stderr, "Error: 'continue' used outside of loop\n");
        return NULL;
    }
    return LLVMBuildBr(ctx->builder, target.continueBB);
}


typedef struct {
    long long value;
    LLVMValueRef constVal;
    ASTNode* caseNode;
} CGCaseEntry;

static bool cg_const_int_from_value(LLVMValueRef v, long long* out) {
    if (!v || LLVMGetTypeKind(LLVMTypeOf(v)) != LLVMIntegerTypeKind) return false;
    if (!LLVMIsConstant(v)) return false;
    if (out) *out = (long long)LLVMConstIntGetSExtValue(v);
    return true;
}

LLVMValueRef codegenSwitch(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_SWITCH) {
        fprintf(stderr, "Error: Invalid node type for codegenSwitch\n");
        return NULL;
    }

    LLVMValueRef condition = codegenNode(ctx, node->switchStmt.condition);
    if (!condition) {
        fprintf(stderr, "Error: Failed to generate switch condition\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef switchEnd = LLVMAppendBasicBlock(func, "switch.end");
    LLVMBasicBlockRef defaultBB = LLVMAppendBasicBlock(func, "switch.default");
    LLVMTypeRef conditionType = LLVMTypeOf(condition);
    bool conditionIsInteger = LLVMGetTypeKind(conditionType) == LLVMIntegerTypeKind;
    if (!conditionIsInteger) {
        // Fallback: avoid jump-table lowering; compare as intptr_t.
        LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
        if (LLVMGetTypeKind(conditionType) == LLVMPointerTypeKind) {
            condition = LLVMBuildPtrToInt(ctx->builder, condition, intptrTy, "switch.ptrtoint");
        } else {
            condition = LLVMBuildBitCast(ctx->builder, condition, intptrTy, "switch.to.int");
        }
        conditionType = LLVMTypeOf(condition);
    }

    size_t caseCount = node->switchStmt.caseListSize;
    CGCaseEntry* entries = calloc(caseCount, sizeof(CGCaseEntry));
    size_t realCases = 0;
    ASTNode* defaultCase = NULL;
    for (size_t i = 0; i < caseCount; ++i) {
        ASTNode* caseNode = node->switchStmt.caseList[i];
        if (!caseNode || caseNode->type != AST_CASE) continue;
        if (!caseNode->caseStmt.caseValue) {
            defaultCase = caseNode;
            continue;
        }
        LLVMValueRef caseValue = codegenNode(ctx, caseNode->caseStmt.caseValue);
        if (!caseValue) continue;
        if (LLVMTypeOf(caseValue) != conditionType) {
            caseValue = cg_cast_value(ctx, caseValue, conditionType, NULL, NULL, "case.cast");
        }
        long long val = 0;
        if (!cg_const_int_from_value(caseValue, &val)) continue;
        entries[realCases].value = val;
        entries[realCases].constVal = caseValue;
        entries[realCases].caseNode = caseNode;
        realCases++;
    }

    long long minVal = 0, maxVal = 0;
    if (realCases > 0) {
        minVal = maxVal = entries[0].value;
        for (size_t i = 1; i < realCases; ++i) {
            if (entries[i].value < minVal) minVal = entries[i].value;
            if (entries[i].value > maxVal) maxVal = entries[i].value;
        }
    }

    bool dense = false;
    if (realCases > 0) {
        long long span = maxVal - minVal + 1;
        dense = conditionIsInteger && span > 0 && span <= (long long)(realCases * 2);
    }

    cg_loop_push(ctx, switchEnd, NULL);
    LLVMBasicBlockRef pendingFallthrough = NULL;

    if (dense) {
        LLVMValueRef switchInst = LLVMBuildSwitch(ctx->builder,
                                                  condition,
                                                  defaultBB,
                                                  (unsigned)realCases);
        for (size_t i = 0; i < realCases; ++i) {
            ASTNode* caseNode = entries[i].caseNode;
            LLVMBasicBlockRef caseBB = LLVMAppendBasicBlock(func, "switch.case");
            LLVMAddCase(switchInst, entries[i].constVal, caseBB);
            if (pendingFallthrough) {
                LLVMPositionBuilderAtEnd(ctx->builder, pendingFallthrough);
                LLVMBuildBr(ctx->builder, caseBB);
                pendingFallthrough = NULL;
            }
            LLVMPositionBuilderAtEnd(ctx->builder, caseBB);
            for (size_t stmtIdx = 0; stmtIdx < caseNode->caseStmt.caseBodySize; ++stmtIdx) {
                codegenNode(ctx, caseNode->caseStmt.caseBody[stmtIdx]);
            }
            LLVMBasicBlockRef currentBB = LLVMGetInsertBlock(ctx->builder);
            if (currentBB && !LLVMGetBasicBlockTerminator(currentBB)) {
                pendingFallthrough = currentBB;
            }
        }
    } else {
        LLVMBasicBlockRef nextTest = LLVMGetInsertBlock(ctx->builder);
        for (size_t i = 0; i < realCases; ++i) {
            ASTNode* caseNode = entries[i].caseNode;
            LLVMBasicBlockRef caseBB = LLVMAppendBasicBlock(func, "switch.case");
            LLVMBasicBlockRef contBB = LLVMAppendBasicBlock(func, "switch.next");
            if (pendingFallthrough) {
                LLVMPositionBuilderAtEnd(ctx->builder, pendingFallthrough);
                LLVMBuildBr(ctx->builder, caseBB);
                pendingFallthrough = NULL;
            }
            LLVMPositionBuilderAtEnd(ctx->builder, nextTest);
            LLVMValueRef cmp = LLVMBuildICmp(ctx->builder,
                                             LLVMIntEQ,
                                             condition,
                                             entries[i].constVal,
                                             "switch.cmp");
            LLVMBuildCondBr(ctx->builder, cmp, caseBB, contBB);

            LLVMPositionBuilderAtEnd(ctx->builder, caseBB);
            for (size_t stmtIdx = 0; stmtIdx < caseNode->caseStmt.caseBodySize; ++stmtIdx) {
                codegenNode(ctx, caseNode->caseStmt.caseBody[stmtIdx]);
            }
            LLVMBasicBlockRef currentBB = LLVMGetInsertBlock(ctx->builder);
            if (currentBB && !LLVMGetBasicBlockTerminator(currentBB)) {
                pendingFallthrough = currentBB;
            }
            nextTest = contBB;
        }
        LLVMPositionBuilderAtEnd(ctx->builder, nextTest);
        LLVMBuildBr(ctx->builder, defaultBB);
    }

    free(entries);

    if (pendingFallthrough) {
        LLVMPositionBuilderAtEnd(ctx->builder, pendingFallthrough);
        LLVMBuildBr(ctx->builder, defaultCase ? defaultBB : switchEnd);
        pendingFallthrough = NULL;
    }

    LLVMPositionBuilderAtEnd(ctx->builder, defaultBB);
    if (defaultCase) {
        for (size_t stmtIdx = 0; stmtIdx < defaultCase->caseStmt.caseBodySize; ++stmtIdx) {
            codegenNode(ctx, defaultCase->caseStmt.caseBody[stmtIdx]);
        }
    }
    if (!LLVMGetBasicBlockTerminator(defaultBB)) {
        LLVMBuildBr(ctx->builder, switchEnd);
    }

    cg_loop_pop(ctx);

    LLVMPositionBuilderAtEnd(ctx->builder, switchEnd);
    return NULL;
}


LLVMValueRef codegenLabel(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_LABEL_DECLARATION) {
        fprintf(stderr, "Error: Invalid node for label codegen\n");
        return NULL;
    }
    const char* name = node->label.labelName;
    if (!name) {
        fprintf(stderr, "Error: label missing name\n");
        return NULL;
    }

    LabelBinding* binding = cg_ensure_label(ctx, name);
    if (!binding) {
        fprintf(stderr, "Error: Failed to allocate label binding\n");
        return NULL;
    }

    LLVMBasicBlockRef insertBlock = LLVMGetInsertBlock(ctx->builder);
    LLVMValueRef func = insertBlock ? LLVMGetBasicBlockParent(insertBlock) : NULL;
    LLVMBasicBlockRef block = cg_label_ensure_block(ctx, binding, func);
    if (!block) {
        fprintf(stderr, "Error: Failed to create label block\n");
        return NULL;
    }

    binding->defined = true;

    if (insertBlock && insertBlock != block && !LLVMGetBasicBlockTerminator(insertBlock)) {
        LLVMBuildBr(ctx->builder, block);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, block);
    if (node->label.statement) {
        codegenNode(ctx, node->label.statement);
    }
    return NULL;
}


LLVMValueRef codegenGoto(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_GOTO_STATEMENT) {
        fprintf(stderr, "Error: Invalid node for goto codegen\n");
        return NULL;
    }
    const char* name = node->gotoStmt.label;
    if (!name) {
        fprintf(stderr, "Error: goto missing label name\n");
        return NULL;
    }

    LabelBinding* binding = cg_ensure_label(ctx, name);
    if (!binding) {
        fprintf(stderr, "Error: Failed to allocate goto label binding\n");
        return NULL;
    }

    LLVMBasicBlockRef insertBlock = LLVMGetInsertBlock(ctx->builder);
    LLVMValueRef func = insertBlock ? LLVMGetBasicBlockParent(insertBlock) : NULL;
    LLVMBasicBlockRef target = cg_label_ensure_block(ctx, binding, func);
    if (!target) {
        fprintf(stderr, "Error: Failed to materialize goto target block\n");
        return NULL;
    }

    LLVMBuildBr(ctx->builder, target);

    if (func) {
        LLVMBasicBlockRef sink = LLVMAppendBasicBlock(func, "goto.sink");
        LLVMPositionBuilderAtEnd(ctx->builder, sink);
    }
    return NULL;
}
