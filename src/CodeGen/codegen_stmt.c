// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include "codegen_types.h"
#include "Syntax/const_eval.h"
#include "Syntax/scope.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static bool cg_attr_payload_has_word(const char* payload, const char* word) {
    if (!payload || !word || !word[0]) {
        return false;
    }
    size_t wlen = strlen(word);
    const char* p = payload;
    while ((p = strstr(p, word)) != NULL) {
        char left = (p == payload) ? '\0' : p[-1];
        char right = p[wlen];
        bool leftOk = (left == '\0') || (!isalnum((unsigned char)left) && left != '_');
        bool rightOk = (right == '\0') || (!isalnum((unsigned char)right) && right != '_');
        if (leftOk && rightOk) {
            return true;
        }
        p += wlen;
    }
    return false;
}

static bool cg_function_has_gnu_weak_attribute(const ASTNode* node) {
    if (!node || node->attributeCount == 0 || !node->attributes) {
        return false;
    }
    for (size_t i = 0; i < node->attributeCount; ++i) {
        const ASTAttribute* attr = node->attributes[i];
        if (!attr || attr->syntax != AST_ATTRIBUTE_SYNTAX_GNU || !attr->payload) {
            continue;
        }
        if (cg_attr_payload_has_word(attr->payload, "weak")) {
            return true;
        }
    }
    return false;
}

static bool cg_extract_constant_int(LLVMValueRef value, long long* outValue) {
    if (!value || !outValue) return false;
    LLVMTypeRef valueType = LLVMTypeOf(value);
    if (!valueType || LLVMGetTypeKind(valueType) != LLVMIntegerTypeKind) {
        return false;
    }

    if (LLVMIsAConstantInt(value)) {
        *outValue = (long long)LLVMConstIntGetSExtValue(value);
        return true;
    }
    return false;
}

static bool cg_add_eval_const_symbol(Scope* scope, const char* name, long long value) {
    if (!scope || !name || name[0] == '\0') return false;
    if (lookupSymbol(&scope->table, name)) {
        return true;
    }

    Symbol* sym = (Symbol*)calloc(1, sizeof(Symbol));
    if (!sym) return false;
    sym->name = strdup(name);
    if (!sym->name) {
        free(sym);
        return false;
    }
    sym->kind = SYMBOL_VARIABLE;
    sym->hasConstValue = true;
    sym->constValue = value;
    sym->type.kind = TYPE_PRIMITIVE;
    sym->type.primitiveType = TOKEN_INT;
    sym->type.isUnsigned = false;

    if (!addToScope(scope, sym)) {
        free(sym->name);
        free(sym);
        return false;
    }
    return true;
}

static void cg_seed_eval_scope_from_codegen_scope(CodegenContext* ctx, Scope* evalScope) {
    if (!ctx || !evalScope) return;
    for (CGScope* scope = ctx->currentScope; scope; scope = scope->parent) {
        for (size_t i = 0; i < scope->count; ++i) {
            const NamedValue* entry = &scope->entries[i];
            if (!entry->name || !entry->addressOnly || !entry->value) {
                continue;
            }
            long long value = 0;
            if (!cg_extract_constant_int(entry->value, &value)) {
                continue;
            }
            (void)cg_add_eval_const_symbol(evalScope, entry->name, value);
        }
    }
}

static bool cg_eval_enum_explicit_value(CodegenContext* ctx,
                                        Scope* evalScope,
                                        ASTNode* valueExpr,
                                        long long* outValue) {
    if (!ctx || !valueExpr || !outValue) return false;

    if (evalScope && constEvalInteger(valueExpr, evalScope, outValue, true)) {
        return true;
    }

    LLVMValueRef value = codegenNode(ctx, valueExpr);
    return cg_extract_constant_int(value, outValue);
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

static bool cg_builder_block_terminated(const CodegenContext* ctx) {
    if (!ctx || !ctx->builder) {
        return true;
    }
    LLVMBasicBlockRef block = LLVMGetInsertBlock(ctx->builder);
    return block && LLVMGetBasicBlockTerminator(block) != NULL;
}

static bool cg_statement_can_reopen_block(const ASTNode* stmt) {
    return stmt && stmt->type == AST_LABEL_DECLARATION;
}

static bool cg_should_emit_statement_in_current_block(const CodegenContext* ctx, const ASTNode* stmt) {
    if (!cg_builder_block_terminated(ctx)) {
        return true;
    }
    return cg_statement_can_reopen_block(stmt);
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

    LLVMValueRef slot = cg_build_entry_alloca(ctx, valueType, "stmt.expr.result.slot");
    LLVMBuildStore(ctx->builder, value, slot);
    return LLVMBuildLoad2(ctx->builder, valueType, slot, "stmt.expr.result");
}

LLVMValueRef codegenProgram(CodegenContext* ctx, ASTNode* node) {
    ProfilerScope scope = profiler_begin("codegen_program");
    if (node->type != AST_PROGRAM) {
        fprintf(stderr, "Error: Invalid node type for codegenProgram\n");
        profiler_end(scope);
        return NULL;
    }

    CG_DEBUG("[CG] Program statements=%zu\n", node->block.statementCount);
    LLVMValueRef last = NULL;
    for (size_t i = 0; i < node->block.statementCount; i++) {
        last = codegenNode(ctx, node->block.statements[i]);
    }
    profiler_end(scope);
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
        ASTNode* stmt = node->block.statements[i];
        if (!cg_should_emit_statement_in_current_block(ctx, stmt)) {
            continue;
        }
        last = codegenNode(ctx, stmt);
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
            if (!cg_should_emit_statement_in_current_block(ctx, stmt)) {
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

LLVMValueRef codegenFunctionDefinition(CodegenContext* ctx, ASTNode* node) {
    ProfilerScope scope = profiler_begin("codegen_function_definition");
    profiler_record_value("codegen_count_function_definition", 1);
#define CG_FUNCTION_RETURN(value) \
    do {                          \
        profiler_end(scope);      \
        return (value);           \
    } while (0)
    if (node->type != AST_FUNCTION_DEFINITION) {
        fprintf(stderr, "Error: Invalid node type for codegenFunctionDefinition\n");
        CG_FUNCTION_RETURN(NULL);
    }

    const char* dbgRun = getenv("DEBUG_RUN");
    const char* fnLabel = (node->functionDef.funcName && node->functionDef.funcName->valueNode.value)
        ? node->functionDef.funcName->valueNode.value
        : "<anon>";
    if (dbgRun) {
        fprintf(stderr, "[CG] enter function %s\n", fnLabel);
    }

    CG_DEBUG("[CG] Function definition paramCount=%zu\n", node->functionDef.paramCount);
    LLVMTypeRef returnType = NULL;
    CGParamInfo* paramInfos = NULL;
    size_t paramCount = 0;
    ParsedType* adjustedParamTypes = NULL;
    LLVMTypeRef* paramTypes = NULL;
    LLVMTypeRef* paramValueTypes = NULL;
    bool* paramPassIndirect = NULL;
    bool useVariadicSRet = false;
    LLVMTypeRef functionIRReturnType = NULL;
    size_t functionParamIndexOffset = 0;
    {
        ProfilerScope returnTypeScope = profiler_begin("codegen_function_definition_return_type");
        returnType = cg_type_from_parsed(ctx, &node->functionDef.returnType);
        if (returnType && LLVMGetTypeKind(returnType) == LLVMFunctionTypeKind) {
            returnType = LLVMPointerType(returnType, 0);
        } else if (returnType && LLVMGetTypeKind(returnType) == LLVMArrayTypeKind) {
            returnType = LLVMPointerType(returnType, 0);
        }
        if (!returnType) {
            returnType = LLVMVoidTypeInContext(ctx->llvmContext);
        }
        returnType = cg_coerce_function_return_type(ctx, returnType);
        useVariadicSRet = cg_should_lower_variadic_sret(ctx,
                                                        returnType,
                                                        node->functionDef.isVariadic);
        functionIRReturnType = useVariadicSRet
            ? LLVMVoidTypeInContext(ctx->llvmContext)
            : returnType;
        functionParamIndexOffset = useVariadicSRet ? 1u : 0u;
        profiler_end(returnTypeScope);
    }
    {
        ProfilerScope paramSignatureScope = profiler_begin("codegen_function_definition_param_signature");
        paramCount = cg_expand_parameters(node->functionDef.parameters,
                                          node->functionDef.paramCount,
                                          &paramInfos,
                                          NULL);
        if (paramCount > 0) {
            adjustedParamTypes = (ParsedType*)calloc(paramCount, sizeof(ParsedType));
            paramTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * paramCount);
            paramValueTypes = (LLVMTypeRef*)calloc(paramCount, sizeof(LLVMTypeRef));
            paramPassIndirect = (bool*)calloc(paramCount, sizeof(bool));
            if (!paramTypes || !adjustedParamTypes || !paramValueTypes || !paramPassIndirect) {
                if (adjustedParamTypes) {
                    free(adjustedParamTypes);
                }
                if (paramTypes) {
                    free(paramTypes);
                }
                if (paramValueTypes) {
                    free(paramValueTypes);
                }
                if (paramPassIndirect) {
                    free(paramPassIndirect);
                }
                cg_free_param_infos(paramInfos);
                profiler_end(paramSignatureScope);
                CG_FUNCTION_RETURN(NULL);
            }
            for (size_t i = 0; i < paramCount; i++) {
                const ParsedType* paramType = paramInfos[i].parsedType;
                adjustedParamTypes[i] = parsedTypeClone(paramType);
                bool passIndirect = false;
                LLVMTypeRef valueType = NULL;
                LLVMTypeRef inferred = cg_lower_parameter_type(ctx,
                                                               &adjustedParamTypes[i],
                                                               &passIndirect,
                                                               &valueType);
                if (!inferred || LLVMGetTypeKind(inferred) == LLVMVoidTypeKind) {
                    inferred = LLVMInt32TypeInContext(ctx->llvmContext);
                }
                paramTypes[i] = inferred;
                paramValueTypes[i] = valueType;
                paramPassIndirect[i] = passIndirect;
            }
        }
        profiler_end(paramSignatureScope);
    }

    LLVMTypeRef* functionParamTypes = paramTypes;
    size_t functionParamCount = paramCount;
    if (useVariadicSRet) {
        functionParamTypes = (LLVMTypeRef*)calloc(paramCount + 1u, sizeof(LLVMTypeRef));
        if (!functionParamTypes) {
            cg_free_param_infos(paramInfos);
            free(paramTypes);
            free(paramValueTypes);
            free(paramPassIndirect);
            if (adjustedParamTypes) {
                for (size_t i = 0; i < paramCount; ++i) {
                    parsedTypeFree(&adjustedParamTypes[i]);
                }
                free(adjustedParamTypes);
            }
            CG_FUNCTION_RETURN(NULL);
        }
        functionParamTypes[0] = LLVMPointerType(returnType, 0);
        for (size_t i = 0; i < paramCount; ++i) {
            functionParamTypes[i + 1u] = paramTypes[i];
        }
        functionParamCount = paramCount + 1u;
    }

    LLVMTypeRef funcType = LLVMFunctionType(functionIRReturnType,
                                            functionParamTypes,
                                            (unsigned)functionParamCount,
                                            node->functionDef.isVariadic ? 1 : 0);
    if (useVariadicSRet) {
        free(functionParamTypes);
    }
    const char* fnName = node->functionDef.funcName->valueNode.value;
    LLVMValueRef function = NULL;
    {
        ProfilerScope symbolBindScope = profiler_begin("codegen_function_definition_symbol_bind");
        function = LLVMGetNamedFunction(ctx->module, fnName);
        if (function) {
            // If a prototype already exists, reuse it; otherwise fall back to a fresh function.
            // Best-effort type nudge: replace the function's type if it mismatches.
            LLVMTypeRef existingType = LLVMGlobalGetValueType(function);
            LLVMTypeRef valueType = LLVMTypeOf(function);
            if (!existingType || LLVMGetTypeKind(existingType) != LLVMFunctionTypeKind) {
                if (valueType && LLVMGetTypeKind(valueType) == LLVMFunctionTypeKind) {
                    existingType = valueType;
                } else {
                    existingType = cg_dereference_ptr_type(ctx, valueType, "function redeclaration");
                }
            }
            bool opaqueFallback =
                (!existingType || LLVMGetTypeKind(existingType) != LLVMFunctionTypeKind);
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
        {
            const SemanticModel* model = cg_context_get_semantic_model(ctx);
            const Symbol* fnSym = model ? semanticModelLookupGlobal(model, fnName) : NULL;
            if (fnSym && (fnSym->linkage == LINKAGE_INTERNAL || fnSym->storage == STORAGE_STATIC)) {
                LLVMSetLinkage(function, LLVMInternalLinkage);
            } else if (node->functionDef.returnType.isInline) {
                /* Header inline defs can appear in many TUs; avoid strong duplicate symbols. */
                LLVMSetLinkage(function, LLVMLinkOnceODRLinkage);
            }
            if (cg_function_has_gnu_weak_attribute(node) &&
                LLVMGetLinkage(function) == LLVMExternalLinkage) {
                LLVMSetLinkage(function, LLVMWeakAnyLinkage);
            }
        }
        if (useVariadicSRet) {
            unsigned sretKind = LLVMGetEnumAttributeKindForName("sret", 4);
            if (sretKind != 0) {
                LLVMAttributeRef sretAttr = LLVMCreateTypeAttribute(ctx->llvmContext,
                                                                    sretKind,
                                                                    returnType);
                LLVMAddAttributeAtIndex(function, 1, sretAttr);
            }
        }
        profiler_end(symbolBindScope);
    }

    LLVMBasicBlockRef entry = NULL;
    const ParsedType* previousReturnType = ctx->currentFunctionReturnType;
    const char* previousFunctionName = ctx->currentFunctionName;
    bool previousUsesVariadicSRet = ctx->currentFunctionUsesVariadicSRet;
    LLVMValueRef previousVariadicSRetPtr = ctx->currentFunctionVariadicSRetPtr;
    LLVMTypeRef previousVariadicSRetType = ctx->currentFunctionVariadicSRetType;
    LLVMValueRef functionVariadicSRetParam = NULL;
    {
        ProfilerScope frameSetupScope = profiler_begin("codegen_function_definition_frame_setup");
        entry = LLVMAppendBasicBlock(function, "entry");
        LLVMPositionBuilderAtEnd(ctx->builder, entry);
        ctx->currentFunctionReturnType = &node->functionDef.returnType;
        ctx->currentFunctionName = fnName;
        ctx->currentFunctionUsesVariadicSRet = useVariadicSRet;
        ctx->currentFunctionVariadicSRetType = returnType;
        if (useVariadicSRet) {
            functionVariadicSRetParam = LLVMGetParam(function, 0);
            ctx->currentFunctionVariadicSRetPtr = functionVariadicSRetParam;
        } else {
            ctx->currentFunctionVariadicSRetPtr = NULL;
        }

        // Labels are function-local in C; drop bindings from the previous function.
        cg_clear_labels(ctx);
        cg_scope_push(ctx);
        profiler_end(frameSetupScope);
    }

    {
        ProfilerScope paramMaterializeScope = profiler_begin("codegen_function_definition_param_materialize");
        for (size_t i = 0; i < paramCount; i++) {
            CGParamInfo* info = &paramInfos[i];
            ASTNode* nameNode = info->nameNode;
            const char* label =
                (nameNode && nameNode->type == AST_IDENTIFIER && nameNode->valueNode.value)
                    ? nameNode->valueNode.value
                    : "param";
            LLVMTypeRef paramType = paramTypes ? paramTypes[i] : LLVMInt32TypeInContext(ctx->llvmContext);
            const ParsedType* storedType = adjustedParamTypes
                ? &adjustedParamTypes[i]
                : (info->parsedType ? info->parsedType : &info->declaration->varDecl.declaredType);
            LLVMValueRef allocaInst = NULL;
            LLVMValueRef incomingParam =
                LLVMGetParam(function, (unsigned)(i + functionParamIndexOffset));
            if (paramPassIndirect && paramPassIndirect[i] && paramValueTypes && paramValueTypes[i]) {
                LLVMTypeRef valueTy = paramValueTypes[i];
                allocaInst = cg_build_entry_alloca(ctx, valueTy, label);
                LLVMTypeRef expectedPtrTy = LLVMPointerType(valueTy, 0);
                if (LLVMTypeOf(incomingParam) != expectedPtrTy) {
                    incomingParam = LLVMBuildBitCast(ctx->builder,
                                                     incomingParam,
                                                     expectedPtrTy,
                                                     "param.byval.cast");
                }
                uint64_t paramSize = 0;
                uint32_t paramAlign = 0;
                if (!cg_size_align_for_type(ctx, storedType, valueTy, &paramSize, &paramAlign) || paramSize == 0) {
                    LLVMTargetDataRef td = ctx && ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
                    if (td) {
                        paramSize = LLVMABISizeOfType(td, valueTy);
                        paramAlign = (uint32_t)LLVMABIAlignmentOfType(td, valueTy);
                    }
                }
                if (paramAlign == 0) {
                    paramAlign = 1;
                }
                LLVMValueRef copySize = LLVMConstInt(cg_get_intptr_type(ctx), (unsigned long long)paramSize, 0);
                LLVMBuildMemCpy(ctx->builder,
                                allocaInst,
                                paramAlign,
                                incomingParam,
                                paramAlign,
                                copySize);
            } else {
                allocaInst = cg_build_entry_alloca(ctx, paramType, label);
                LLVMBuildStore(ctx->builder, incomingParam, allocaInst);
            }
            LLVMTypeRef elementLLVM = cg_element_type_from_pointer_parsed(ctx, storedType);
            cg_scope_insert(ctx->currentScope,
                            label,
                            allocaInst,
                            (paramPassIndirect && paramPassIndirect[i] && paramValueTypes && paramValueTypes[i])
                                ? paramValueTypes[i]
                                : paramType,
                            false,
                            elementLLVM != NULL /* addressOnly */ ? false : false,
                            elementLLVM,
                            storedType);
        }
        profiler_end(paramMaterializeScope);
    }

    {
        ProfilerScope bodyScope = profiler_begin("codegen_function_definition_body");
        if (dbgRun) fprintf(stderr, "[CG] body begin %s\n", fnLabel);
        codegenNode(ctx, node->functionDef.body);
        if (dbgRun) fprintf(stderr, "[CG] body end %s\n", fnLabel);
        profiler_end(bodyScope);
    }

    {
        ProfilerScope finalizeScope = profiler_begin("codegen_function_definition_finalize");
        // Ensure function IR is always well-formed: C allows falling off the end of
        // void functions, so emit an implicit terminator when the final block is open.
        LLVMBasicBlockRef finalBB = LLVMGetInsertBlock(ctx->builder);
        if (finalBB && !LLVMGetBasicBlockTerminator(finalBB)) {
            if (useVariadicSRet && functionVariadicSRetParam && returnType &&
                LLVMGetTypeKind(returnType) != LLVMVoidTypeKind) {
                LLVMTypeRef expectedPtrTy = LLVMPointerType(returnType, 0);
                LLVMValueRef sretPtr = functionVariadicSRetParam;
                if (LLVMTypeOf(sretPtr) != expectedPtrTy) {
                    sretPtr = LLVMBuildBitCast(ctx->builder,
                                               sretPtr,
                                               expectedPtrTy,
                                               "ret.sret.cast");
                }
                LLVMBuildStore(ctx->builder, LLVMConstNull(returnType), sretPtr);
                LLVMBuildRetVoid(ctx->builder);
            } else if (returnType && LLVMGetTypeKind(returnType) != LLVMVoidTypeKind) {
                LLVMBuildRet(ctx->builder, LLVMConstNull(returnType));
            } else {
                LLVMBuildRetVoid(ctx->builder);
            }
        }
        profiler_end(finalizeScope);
    }

    cg_scope_pop(ctx);
    cg_clear_labels(ctx);
    ctx->currentFunctionReturnType = previousReturnType;
    ctx->currentFunctionName = previousFunctionName;
    ctx->currentFunctionUsesVariadicSRet = previousUsesVariadicSRet;
    ctx->currentFunctionVariadicSRetPtr = previousVariadicSRetPtr;
    ctx->currentFunctionVariadicSRetType = previousVariadicSRetType;
    cg_free_param_infos(paramInfos);
    free(paramTypes);
    free(paramValueTypes);
    free(paramPassIndirect);
    if (adjustedParamTypes) {
        for (size_t i = 0; i < paramCount; ++i) {
            parsedTypeFree(&adjustedParamTypes[i]);
        }
        free(adjustedParamTypes);
    }
    if (ctx->verifyFunctions && function) {
        ProfilerScope verifyScope = profiler_begin("codegen_function_definition_verify");
        if (LLVMVerifyFunction(function, LLVMPrintMessageAction) != 0) {
            fprintf(stderr, "Warning: LLVM verification failed for function %s\n",
                    node->functionDef.funcName && node->functionDef.funcName->valueNode.value
                        ? node->functionDef.funcName->valueNode.value
                        : "<anonymous>");
        }
        profiler_end(verifyScope);
    }
    if (dbgRun) {
        fprintf(stderr, "[CG] exit function %s\n", fnLabel);
    }
    CG_FUNCTION_RETURN(function);
#undef CG_FUNCTION_RETURN
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
    if (!ctx->currentScope || !node->enumDef.members || node->enumDef.memberCount == 0) {
        return NULL;
    }

    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    Scope* globalScope = model ? semanticModelGetGlobalScope(model) : NULL;
    Scope* evalScope = createScope(globalScope);
    if (evalScope) {
        cg_seed_eval_scope_from_codegen_scope(ctx, evalScope);
    }

    long long currentValue = 0;
    bool haveCurrentValue = false;
    LLVMTypeRef enumValueType = LLVMInt32TypeInContext(ctx->llvmContext);

    for (size_t i = 0; i < node->enumDef.memberCount; ++i) {
        ASTNode* member = node->enumDef.members[i];
        if (!member || member->type != AST_IDENTIFIER || !member->valueNode.value) {
            continue;
        }

        ASTNode* explicitExpr = (node->enumDef.values && i < node->enumDef.memberCount)
            ? node->enumDef.values[i]
            : NULL;
        if (explicitExpr) {
            long long explicitValue = 0;
            if (!cg_eval_enum_explicit_value(ctx, evalScope, explicitExpr, &explicitValue)) {
                fprintf(stderr,
                        "Error: Failed to evaluate enum constant %s\n",
                        member->valueNode.value);
                if (!haveCurrentValue) {
                    currentValue = 0;
                } else {
                    currentValue += 1;
                }
            } else {
                currentValue = explicitValue;
            }
            haveCurrentValue = true;
        } else if (!haveCurrentValue) {
            currentValue = 0;
            haveCurrentValue = true;
        } else {
            currentValue += 1;
        }

        LLVMValueRef constValue = LLVMConstInt(enumValueType, (unsigned long long)currentValue, 1);
        cg_scope_insert(ctx->currentScope,
                        member->valueNode.value,
                        constValue,
                        enumValueType,
                        false,
                        true,
                        NULL,
                        NULL);
        if (evalScope) {
            (void)cg_add_eval_const_symbol(evalScope, member->valueNode.value, currentValue);
        }
    }

    if (evalScope) {
        destroyScope(evalScope);
    }

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
        declaredReturnLLVM = cg_coerce_function_return_type(ctx, declaredReturnLLVM);
    }

    if (node->returnStmt.returnValue) {
        LLVMValueRef value = codegenNode(ctx, node->returnStmt.returnValue);
        if (!value) {
            fprintf(stderr, "Error: Failed to generate return value\n");
            return NULL;
        }

        if (declaredReturnLLVM && LLVMGetTypeKind(declaredReturnLLVM) != LLVMVoidTypeKind) {
            if (LLVMTypeOf(value) != declaredReturnLLVM) {
                LLVMTypeKind valueKind = LLVMGetTypeKind(LLVMTypeOf(value));
                LLVMTypeKind retKind = LLVMGetTypeKind(declaredReturnLLVM);
                if ((valueKind == LLVMStructTypeKind || valueKind == LLVMArrayTypeKind) &&
                    (retKind == LLVMIntegerTypeKind || retKind == LLVMArrayTypeKind)) {
                    value = cg_pack_aggregate_for_abi_return(ctx,
                                                             value,
                                                             declaredReturnLLVM,
                                                             "return.abi.pack");
                } else {
                    value = cg_cast_value(ctx,
                                          value,
                                          declaredReturnLLVM,
                                          NULL,
                                          ctx->currentFunctionReturnType,
                                          "return.cast");
                }
            }
        }

        if (ctx->currentFunctionUsesVariadicSRet &&
            ctx->currentFunctionVariadicSRetPtr &&
            declaredReturnLLVM &&
            LLVMGetTypeKind(declaredReturnLLVM) != LLVMVoidTypeKind) {
            LLVMTypeRef expectedPtrTy = LLVMPointerType(declaredReturnLLVM, 0);
            LLVMValueRef sretPtr = ctx->currentFunctionVariadicSRetPtr;
            if (LLVMTypeOf(sretPtr) != expectedPtrTy) {
                sretPtr = LLVMBuildBitCast(ctx->builder,
                                           sretPtr,
                                           expectedPtrTy,
                                           "return.sret.cast");
            }
            LLVMBuildStore(ctx->builder, value, sretPtr);
            CG_DEBUG("[CG] Return emitting variadic sret value\n");
            return LLVMBuildRetVoid(ctx->builder);
        }

        CG_DEBUG("[CG] Return emitting with value\n");
        return LLVMBuildRet(ctx->builder, value);
    }

    if (declaredReturnLLVM && LLVMGetTypeKind(declaredReturnLLVM) != LLVMVoidTypeKind) {
        fprintf(stderr, "Error: Non-void function missing return value; defaulting to zero\n");
        if (ctx->currentFunctionUsesVariadicSRet &&
            ctx->currentFunctionVariadicSRetPtr) {
            LLVMTypeRef expectedPtrTy = LLVMPointerType(declaredReturnLLVM, 0);
            LLVMValueRef sretPtr = ctx->currentFunctionVariadicSRetPtr;
            if (LLVMTypeOf(sretPtr) != expectedPtrTy) {
                sretPtr = LLVMBuildBitCast(ctx->builder,
                                           sretPtr,
                                           expectedPtrTy,
                                           "return.sret.zero.cast");
            }
            LLVMBuildStore(ctx->builder, LLVMConstNull(declaredReturnLLVM), sretPtr);
            return LLVMBuildRetVoid(ctx->builder);
        }
        LLVMValueRef zero = LLVMConstNull(declaredReturnLLVM);
        return LLVMBuildRet(ctx->builder, zero);
    }

    CG_DEBUG("[CG] Return emitting void\n");
    return LLVMBuildRetVoid(ctx->builder);
}
