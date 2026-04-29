// SPDX-License-Identifier: Apache-2.0

#include "codegen_expr_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LLVMValueRef codegenFunctionCall(CodegenContext* ctx, ASTNode* node) {
#define CG_CALL_RETURN(value) \
    do {                      \
        profiler_end(scope);  \
        return (value);       \
    } while (0)
    ProfilerScope scope = profiler_begin("codegen_function_call");
    profiler_record_value("codegen_count_function_call", 1);
    if (node->type != AST_FUNCTION_CALL) {
        fprintf(stderr, "Error: Invalid node type for codegenFunctionCall\n");
        CG_CALL_RETURN(NULL);
    }

    if (!node->functionCall.callee) {
        fprintf(stderr, "Error: Unsupported callee in function call\n");
        CG_CALL_RETURN(NULL);
    }

    const ParsedType* calleeParsed = cg_resolve_expression_type(ctx, node->functionCall.callee);
    const char* calleeName = node->functionCall.callee->type == AST_IDENTIFIER
        ? node->functionCall.callee->valueNode.value
        : NULL;

    if (calleeName &&
        (strcmp(calleeName, "__builtin_offsetof") == 0 ||
         strcmp(calleeName, "offsetof") == 0)) {
        size_t offset = 0;
        bool ok = false;
        ASTNode* offsetofTypeArg = NULL;
        ASTNode* offsetofFieldArg = NULL;
        if (node->functionCall.argumentCount == 2) {
            offsetofTypeArg = node->functionCall.arguments[0];
            offsetofFieldArg = node->functionCall.arguments[1];
        }
        if (offsetofTypeArg &&
            offsetofTypeArg->type == AST_PARSED_TYPE &&
            offsetofFieldArg &&
            offsetofFieldArg->type == AST_IDENTIFIER) {
            ok = cg_eval_builtin_offsetof(ctx,
                                          &offsetofTypeArg->parsedTypeNode.parsed,
                                          offsetofFieldArg->valueNode.value,
                                          &offset);
        }
        const TargetLayout* tl = cg_context_get_target_layout(ctx);
        unsigned bits = (tl && tl->pointerBits) ? (unsigned)tl->pointerBits : 64;
        LLVMTypeRef offTy = LLVMIntTypeInContext(ctx->llvmContext, bits);
        CG_CALL_RETURN(LLVMConstInt(offTy, ok ? (unsigned long long)offset : 0ULL, false));
    }
    if (calleeName && strcmp(calleeName, "__builtin_constant_p") == 0) {
        CG_CALL_RETURN(LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, false));
    }

    LLVMValueRef function = codegenNode(ctx, node->functionCall.callee);
    if (!function) {
        fprintf(stderr, "Error: Undefined function %s\n",
                calleeName ? calleeName : "<expr>");
        CG_CALL_RETURN(NULL);
    }

    bool isVaStartBuiltin = calleeName &&
        (strcmp(calleeName, "va_start") == 0 || strcmp(calleeName, "__builtin_va_start") == 0);
    bool isVaArgBuiltin = calleeName &&
        (strcmp(calleeName, "__builtin_va_arg") == 0 || strcmp(calleeName, "va_arg") == 0);
    bool isVaEndBuiltin = calleeName &&
        (strcmp(calleeName, "va_end") == 0 || strcmp(calleeName, "__builtin_va_end") == 0);
    bool isVaCopyBuiltin = calleeName &&
        (strcmp(calleeName, "__builtin_va_copy") == 0 || strcmp(calleeName, "va_copy") == 0);

    LLVMValueRef* args = NULL;
    if (node->functionCall.argumentCount > 0) {
        args = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * node->functionCall.argumentCount);
        if (!args) CG_CALL_RETURN(NULL);
        for (size_t i = 0; i < node->functionCall.argumentCount; i++) {
            if ((isVaStartBuiltin || isVaArgBuiltin || isVaEndBuiltin) && i == 0) {
                args[i] = NULL;
                continue;
            }
            if (isVaCopyBuiltin && i < 2) {
                args[i] = NULL;
                continue;
            }
            args[i] = codegenNode(ctx, node->functionCall.arguments[i]);
        }
    }

    LLVMTypeRef i8PtrType = LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
    LLVMTypeRef sizeType = cg_get_intptr_type(ctx);
    LLVMTypeRef intType = LLVMInt32TypeInContext(ctx->llvmContext);
    LLVMValueRef builtinResult = NULL;
    if (cg_try_codegen_builtin_call(ctx,
                                    node,
                                    calleeName,
                                    args,
                                    i8PtrType,
                                    sizeType,
                                    intType,
                                    &builtinResult)) {
        return builtinResult;
    }

    const Symbol* sym = cg_lookup_function_symbol_for_callee(ctx, node->functionCall.callee);
    bool noPrototype = sym && sym->kind == SYMBOL_FUNCTION && !sym->signature.hasPrototype;
    bool externalAbiEligible = cg_is_known_external_abi_function_name(calleeName);

    LLVMTypeRef calleeType = NULL;
    if (sym && !noPrototype) {
        calleeType = cg_function_type_from_symbol(ctx, sym);
    }
    if (calleeParsed && !noPrototype) {
        if (!calleeType) {
            calleeType = functionTypeFromPointerParsed(ctx,
                                                       calleeParsed,
                                                       node->functionCall.argumentCount,
                                                       args);
        }
    }
    if (!calleeType && !noPrototype) {
        calleeType = LLVMTypeOf(function);
        if (calleeType && LLVMGetTypeKind(calleeType) == LLVMPointerTypeKind) {
            LLVMTypeRef element = cg_dereference_ptr_type(ctx, calleeType, "call target");
            if (element && LLVMGetTypeKind(element) == LLVMFunctionTypeKind) {
                calleeType = element;
            }
        }
    }
    if ((!calleeType || LLVMGetTypeKind(calleeType) != LLVMFunctionTypeKind) &&
        sym && !noPrototype) {
        calleeType = cg_function_type_from_symbol(ctx, sym);
    }

    size_t argCount = node->functionCall.argumentCount;
    LLVMValueRef* finalArgs = args;
    LLVMValueRef* promotedArgs = NULL;
    LLVMValueRef* externalAbiArgs = NULL;
    LLVMTypeRef* externalAbiParamTypes = NULL;
    bool externalAbiCallAdjusted = false;

    if (noPrototype) {
        if (argCount > 0 && args) {
            promotedArgs = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * argCount);
            if (promotedArgs) {
                for (size_t i = 0; i < argCount; ++i) {
                    promotedArgs[i] = cg_apply_default_promotion(ctx, args[i], node->functionCall.arguments[i]);
                }
                finalArgs = promotedArgs;
            }
        }

        LLVMTypeRef retType = NULL;
        if (sym) {
            retType = cg_type_from_parsed(ctx, &sym->type);
            if (retType && LLVMGetTypeKind(retType) == LLVMFunctionTypeKind) {
                retType = LLVMPointerType(retType, 0);
            } else if (retType && LLVMGetTypeKind(retType) == LLVMArrayTypeKind) {
                retType = LLVMPointerType(retType, 0);
            }
        }
        if (!retType || LLVMGetTypeKind(retType) == LLVMVoidTypeKind) {
            retType = LLVMVoidTypeInContext(ctx->llvmContext);
        }
        retType = cg_coerce_function_return_type(ctx, retType);

        LLVMTypeRef* paramTypes = NULL;
        if (argCount > 0) {
            paramTypes = (LLVMTypeRef*)calloc(argCount, sizeof(LLVMTypeRef));
            if (!paramTypes) CG_CALL_RETURN(NULL);
            for (size_t i = 0; i < argCount; ++i) {
                LLVMValueRef val = finalArgs ? finalArgs[i] : NULL;
                LLVMTypeRef argTy = val ? LLVMTypeOf(val) : LLVMInt32TypeInContext(ctx->llvmContext);
                LLVMTypeRef coercedTy = externalAbiEligible
                    ? cg_external_abi_coerce_param_type(ctx, argTy)
                    : argTy;
                if (externalAbiEligible && coercedTy != argTy && finalArgs && val) {
                    finalArgs[i] = cg_pack_aggregate_for_external_abi(ctx, val, coercedTy, "call.noproto.extabi");
                    val = finalArgs[i];
                    argTy = LLVMTypeOf(val);
                }
                paramTypes[i] = coercedTy ? coercedTy : argTy;
                if (!paramTypes[i]) {
                    paramTypes[i] = LLVMInt32TypeInContext(ctx->llvmContext);
                }
            }
        }
        calleeType = LLVMFunctionType(retType, paramTypes, (unsigned)argCount, 0);
        free(paramTypes);
    }

    if ((!calleeType || LLVMGetTypeKind(calleeType) != LLVMFunctionTypeKind) &&
        function &&
        LLVMGetTypeKind(LLVMTypeOf(function)) == LLVMPointerTypeKind) {
        LLVMTypeRef inferredRet = NULL;
        const ParsedType* inferredRetParsed = cg_refine_function_call_result_type(ctx, node);
        if (!inferredRetParsed) {
            inferredRetParsed = cg_resolve_expression_type(ctx, node);
        }
        if (inferredRetParsed) {
            inferredRet = cg_type_from_parsed(ctx, inferredRetParsed);
            if (inferredRet && LLVMGetTypeKind(inferredRet) == LLVMFunctionTypeKind) {
                inferredRet = LLVMPointerType(inferredRet, 0);
            } else if (inferredRet && LLVMGetTypeKind(inferredRet) == LLVMArrayTypeKind) {
                inferredRet = LLVMPointerType(inferredRet, 0);
            }
        }
        if (!inferredRet) {
            inferredRet = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        inferredRet = cg_coerce_function_return_type(ctx, inferredRet);

        LLVMTypeRef* inferredParams = NULL;
        if (argCount > 0) {
            inferredParams = (LLVMTypeRef*)calloc(argCount, sizeof(LLVMTypeRef));
            if (!inferredParams) {
                if (promotedArgs) free(promotedArgs);
                free(args);
                CG_CALL_RETURN(NULL);
            }
            for (size_t i = 0; i < argCount; ++i) {
                LLVMValueRef val = finalArgs ? finalArgs[i] : NULL;
                inferredParams[i] = val ? LLVMTypeOf(val) : LLVMInt32TypeInContext(ctx->llvmContext);
                if (!inferredParams[i]) {
                    inferredParams[i] = LLVMInt32TypeInContext(ctx->llvmContext);
                }
            }
        }
        calleeType = LLVMFunctionType(inferredRet, inferredParams, (unsigned)argCount, 0);
        free(inferredParams);
        function = LLVMBuildBitCast(ctx->builder,
                                    function,
                                    LLVMPointerType(calleeType, 0),
                                    "call.infer.cast");
    }

    if (!calleeType || LLVMGetTypeKind(calleeType) != LLVMFunctionTypeKind) {
        fprintf(stderr, "Error: call target is not a function type\n");
        if (promotedArgs) free(promotedArgs);
        free(args);
        CG_CALL_RETURN(NULL);
    }

    if (!noPrototype) {
        const ParsedType* refinedReturnParsed = cg_refine_function_call_result_type(ctx, node);
        if (refinedReturnParsed) {
            LLVMTypeRef refinedReturn = cg_type_from_parsed(ctx, refinedReturnParsed);
            if (refinedReturn && LLVMGetTypeKind(refinedReturn) == LLVMFunctionTypeKind) {
                refinedReturn = LLVMPointerType(refinedReturn, 0);
            } else if (refinedReturn && LLVMGetTypeKind(refinedReturn) == LLVMArrayTypeKind) {
                refinedReturn = LLVMPointerType(refinedReturn, 0);
            }
            refinedReturn = cg_coerce_function_return_type(ctx, refinedReturn);
            if (refinedReturn && LLVMGetTypeKind(refinedReturn) != LLVMVoidTypeKind) {
                LLVMTypeRef currentReturn = LLVMGetReturnType(calleeType);
                const ParsedType* semanticCallResult = cg_resolve_expression_type(ctx, node);
                bool semanticResultPointerish = false;
                if (semanticCallResult) {
                    if (semanticCallResult->pointerDepth > 0 || semanticCallResult->isFunctionPointer) {
                        semanticResultPointerish = true;
                    } else {
                        for (size_t d = 0; d < semanticCallResult->derivationCount; ++d) {
                            const TypeDerivation* deriv = parsedTypeGetDerivation(semanticCallResult, d);
                            if (deriv && deriv->kind == TYPE_DERIVATION_POINTER) {
                                semanticResultPointerish = true;
                                break;
                            }
                        }
                    }
                }
                bool unsafePointerDowngrade =
                    semanticResultPointerish &&
                    currentReturn &&
                    refinedReturn &&
                    LLVMGetTypeKind(currentReturn) == LLVMPointerTypeKind &&
                    LLVMGetTypeKind(refinedReturn) != LLVMPointerTypeKind;
                if (!unsafePointerDowngrade && currentReturn != refinedReturn) {
                    unsigned fixedCount = LLVMCountParamTypes(calleeType);
                    LLVMTypeRef* fixedTypes = NULL;
                    if (fixedCount > 0) {
                        fixedTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * fixedCount);
                        if (fixedTypes) {
                            LLVMGetParamTypes(calleeType, fixedTypes);
                        }
                    }
                    bool isVar = LLVMIsFunctionVarArg(calleeType);
                    LLVMTypeRef rebuilt = LLVMFunctionType(refinedReturn, fixedTypes, fixedCount, isVar ? 1 : 0);
                    free(fixedTypes);
                    calleeType = rebuilt;
                    LLVMTypeRef fnPtrType = LLVMPointerType(calleeType, 0);
                    function = LLVMBuildBitCast(ctx->builder, function, fnPtrType, "call.retfix.cast");
                }
            }
        }
    }

    char* resolvedFnType = LLVMPrintTypeToString(calleeType);
    CG_DEBUG("[CG] Function call resolved type: %s\n", resolvedFnType ? resolvedFnType : "<null>");
    if (resolvedFnType) LLVMDisposeMessage(resolvedFnType);
    if (!noPrototype && sym && sym->signature.hasPrototype && sym->signature.paramCount > 0 && args) {
        size_t fixedCount = sym->signature.paramCount;
        if (fixedCount > argCount) {
            fixedCount = argCount;
        }
        for (size_t i = 0; i < fixedCount; ++i) {
            const ParsedType* fromParsed = cg_resolve_expression_type(ctx, node->functionCall.arguments[i]);

            if (externalAbiEligible) {
                LLVMTypeRef extParamTy = cg_type_from_parsed(ctx, &sym->signature.params[i]);
                if (extParamTy && LLVMGetTypeKind(extParamTy) == LLVMFunctionTypeKind) {
                    extParamTy = LLVMPointerType(extParamTy, 0);
                } else if (extParamTy && LLVMGetTypeKind(extParamTy) == LLVMArrayTypeKind) {
                    extParamTy = LLVMPointerType(extParamTy, 0);
                }
                extParamTy = cg_external_abi_coerce_param_type(ctx, extParamTy);
                if (!extParamTy || LLVMGetTypeKind(extParamTy) == LLVMVoidTypeKind) {
                    continue;
                }
                if (args[i]) {
                    LLVMTypeRef argTy = LLVMTypeOf(args[i]);
                    LLVMTypeKind argKind = argTy ? LLVMGetTypeKind(argTy) : LLVMVoidTypeKind;
                    LLVMTypeKind dstKind = LLVMGetTypeKind(extParamTy);
                    if ((argKind == LLVMStructTypeKind || argKind == LLVMArrayTypeKind) &&
                        (dstKind == LLVMIntegerTypeKind || dstKind == LLVMArrayTypeKind) &&
                        argTy != extParamTy) {
                        args[i] = cg_pack_aggregate_for_external_abi(ctx,
                                                                     args[i],
                                                                     extParamTy,
                                                                     "call.arg.extabi.pack");
                    } else {
                        args[i] = cg_cast_value(ctx,
                                                args[i],
                                                extParamTy,
                                                fromParsed,
                                                &sym->signature.params[i],
                                                "call.arg.extabi.cast");
                    }
                }
                continue;
            }

            bool passIndirect = false;
            LLVMTypeRef valueParamTy = NULL;
            LLVMTypeRef paramTy = cg_lower_parameter_type(ctx,
                                                          &sym->signature.params[i],
                                                          &passIndirect,
                                                          &valueParamTy);
            if (!paramTy || LLVMGetTypeKind(paramTy) == LLVMVoidTypeKind) {
                continue;
            }
            if (passIndirect && valueParamTy) {
                LLVMValueRef argVal = args[i];
                if (argVal && LLVMTypeOf(argVal) != valueParamTy) {
                    argVal = cg_cast_value(ctx,
                                           argVal,
                                           valueParamTy,
                                           fromParsed,
                                           &sym->signature.params[i],
                                           "call.arg.byval.cast");
                }
                if (argVal && LLVMTypeOf(argVal) == valueParamTy) {
                    LLVMValueRef tmp = cg_build_entry_alloca(ctx, valueParamTy, "call.arg.byval.tmp");
                    LLVMBuildStore(ctx->builder, argVal, tmp);
                    args[i] = tmp;
                }
            } else {
                if (args[i]) {
                    LLVMTypeRef argTy = LLVMTypeOf(args[i]);
                    LLVMTypeKind argKind = argTy ? LLVMGetTypeKind(argTy) : LLVMVoidTypeKind;
                    LLVMTypeKind dstKind = LLVMGetTypeKind(paramTy);
                    if ((argKind == LLVMStructTypeKind || argKind == LLVMArrayTypeKind) &&
                        (dstKind == LLVMIntegerTypeKind || dstKind == LLVMArrayTypeKind) &&
                        argTy != paramTy) {
                        args[i] = cg_pack_aggregate_for_external_abi(ctx,
                                                                     args[i],
                                                                     paramTy,
                                                                     "call.arg.pack");
                    } else {
                        args[i] = cg_cast_value(ctx,
                                                args[i],
                                                paramTy,
                                                fromParsed,
                                                &sym->signature.params[i],
                                                "call.arg.cast");
                    }
                }
            }
        }
    }
    if (noPrototype) {
        LLVMTypeRef fnPtrTy = LLVMPointerType(calleeType, 0);
        function = LLVMBuildBitCast(ctx->builder, function, fnPtrTy, "call.cast");
    } else {
        bool isVariadic = LLVMIsFunctionVarArg(calleeType);
        unsigned fixedParams = LLVMCountParamTypes(calleeType);

        if (isVariadic && argCount > fixedParams && args) {
            promotedArgs = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * argCount);
            if (promotedArgs) {
                for (size_t i = 0; i < argCount; ++i) {
                    LLVMValueRef val = args[i];
                    if (i >= fixedParams) {
                        val = cg_apply_default_promotion(ctx, val, node->functionCall.arguments[i]);
                    }
                    promotedArgs[i] = val;
                }
                finalArgs = promotedArgs;
            }
        }
    }

    if (!noPrototype &&
        externalAbiEligible) {
        unsigned fixedParamCount = LLVMCountParamTypes(calleeType);
        bool isVariadic = LLVMIsFunctionVarArg(calleeType) != 0;
        if (fixedParamCount > 0) {
            externalAbiParamTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * fixedParamCount);
            if (externalAbiParamTypes) {
                LLVMGetParamTypes(calleeType, externalAbiParamTypes);
                if (argCount > 0) {
                    externalAbiArgs = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * argCount);
                    if (externalAbiArgs) {
                        for (size_t i = 0; i < argCount; ++i) {
                            externalAbiArgs[i] = finalArgs[i];
                        }
                    }
                }

                for (unsigned i = 0; i < fixedParamCount; ++i) {
                    LLVMTypeRef sourceTy = externalAbiParamTypes[i];
                    LLVMTypeRef desiredTy = sourceTy;
                    if (sym && i < sym->signature.paramCount) {
                        LLVMTypeRef parsedTy = cg_type_from_parsed(ctx, &sym->signature.params[i]);
                        if (parsedTy && LLVMGetTypeKind(parsedTy) == LLVMFunctionTypeKind) {
                            parsedTy = LLVMPointerType(parsedTy, 0);
                        } else if (parsedTy && LLVMGetTypeKind(parsedTy) == LLVMArrayTypeKind) {
                            parsedTy = LLVMPointerType(parsedTy, 0);
                        }
                        if (parsedTy && LLVMGetTypeKind(parsedTy) != LLVMVoidTypeKind) {
                            desiredTy = parsedTy;
                        }
                    }
                    LLVMTypeRef coercedTy = cg_external_abi_coerce_param_type(ctx, desiredTy);
                    externalAbiParamTypes[i] = coercedTy;
                    if (coercedTy != sourceTy) {
                        externalAbiCallAdjusted = true;
                    }
                    if (externalAbiArgs && i < argCount) {
                        LLVMValueRef arg = externalAbiArgs[i];
                        if (arg && LLVMTypeOf(arg) != coercedTy) {
                            LLVMTypeKind argKind = LLVMGetTypeKind(LLVMTypeOf(arg));
                            LLVMTypeKind dstKind = LLVMGetTypeKind(coercedTy);
                            if ((argKind == LLVMStructTypeKind || argKind == LLVMArrayTypeKind) &&
                                (dstKind == LLVMIntegerTypeKind || dstKind == LLVMArrayTypeKind)) {
                                arg = cg_pack_aggregate_for_external_abi(ctx, arg, coercedTy, "call.extabi.arg");
                            } else {
                                const ParsedType* fromParsed = NULL;
                                const ParsedType* toParsed = NULL;
                                if (sym && i < sym->signature.paramCount) {
                                    toParsed = &sym->signature.params[i];
                                }
                                if (node->functionCall.arguments && i < node->functionCall.argumentCount) {
                                    fromParsed = cg_resolve_expression_type(ctx, node->functionCall.arguments[i]);
                                }
                                arg = cg_cast_value(ctx,
                                                    arg,
                                                    coercedTy,
                                                    fromParsed,
                                                    toParsed,
                                                    "call.extabi.cast");
                            }
                            externalAbiArgs[i] = arg;
                            externalAbiCallAdjusted = true;
                        }
                    }
                }

                if (externalAbiCallAdjusted) {
                    LLVMTypeRef adjustedType = LLVMFunctionType(LLVMGetReturnType(calleeType),
                                                                externalAbiParamTypes,
                                                                fixedParamCount,
                                                                isVariadic ? 1 : 0);
                    calleeType = adjustedType;
                    function = LLVMBuildBitCast(ctx->builder,
                                                function,
                                                LLVMPointerType(calleeType, 0),
                                                "call.extabi.cast");
                    if (externalAbiArgs) {
                        finalArgs = externalAbiArgs;
                    }
                }
            }
        }
    }

    if (!noPrototype && sym && sym->signature.hasPrototype) {
        LLVMTypeRef signatureCalleeType = cg_function_type_from_symbol(ctx, sym);
        if (signatureCalleeType && LLVMGetTypeKind(signatureCalleeType) == LLVMFunctionTypeKind) {
            calleeType = signatureCalleeType;
        }
    }

    if (!noPrototype && argCount > 0 && finalArgs) {
        unsigned fixedParamCount = LLVMCountParamTypes(calleeType);
        if (fixedParamCount > 0) {
            LLVMTypeRef* paramTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * fixedParamCount);
            if (paramTypes) {
                LLVMGetParamTypes(calleeType, paramTypes);
                size_t castCount = argCount < fixedParamCount ? argCount : fixedParamCount;
                for (size_t i = 0; i < castCount; ++i) {
                    const ParsedType* fromParsed = NULL;
                    const ParsedType* toParsed = NULL;
                    if (node->functionCall.arguments && i < node->functionCall.argumentCount) {
                        fromParsed = cg_resolve_expression_type(ctx, node->functionCall.arguments[i]);
                    }
                    if (sym && i < sym->signature.paramCount) {
                        toParsed = &sym->signature.params[i];
                    }
                    if (finalArgs[i] &&
                        LLVMGetTypeKind(paramTypes[i]) == LLVMPointerTypeKind &&
                        (!toParsed || !cg_parsed_type_is_pointerish(toParsed))) {
                        LLVMTypeRef argTy = LLVMTypeOf(finalArgs[i]);
                        if (argTy) {
                            LLVMTypeKind argKind = LLVMGetTypeKind(argTy);
                            if (argKind == LLVMStructTypeKind || argKind == LLVMArrayTypeKind) {
                                LLVMValueRef tmp = cg_build_entry_alloca(ctx, argTy, "call.arg.byval.indirect.tmp");
                                if (tmp) {
                                    LLVMBuildStore(ctx->builder, finalArgs[i], tmp);
                                    finalArgs[i] = tmp;
                                    continue;
                                }
                            }
                        }
                    }
                    if (finalArgs[i]) {
                        LLVMTypeRef argTy = LLVMTypeOf(finalArgs[i]);
                        if (argTy) {
                            LLVMTypeKind argKind = LLVMGetTypeKind(argTy);
                            LLVMTypeKind paramKind = LLVMGetTypeKind(paramTypes[i]);
                            if ((argKind == LLVMStructTypeKind || argKind == LLVMArrayTypeKind) &&
                                (paramKind == LLVMIntegerTypeKind || paramKind == LLVMArrayTypeKind) &&
                                argTy != paramTypes[i]) {
                                finalArgs[i] = cg_pack_aggregate_for_external_abi(ctx,
                                                                                  finalArgs[i],
                                                                                  paramTypes[i],
                                                                                  "call.arg.abi.pack");
                                continue;
                            }
                        }
                    }
                    finalArgs[i] = cg_prepare_call_argument(ctx,
                                                            finalArgs[i],
                                                            paramTypes[i],
                                                            fromParsed,
                                                            toParsed,
                                                            "call.arg.param.cast");
                }
                free(paramTypes);
            }
        }
    }

    const ParsedType* semanticReturnParsed = cg_resolve_expression_type(ctx, node);
    LLVMTypeRef semanticReturnTy = NULL;
    if (semanticReturnParsed) {
        semanticReturnTy = cg_type_from_parsed(ctx, semanticReturnParsed);
        if (semanticReturnTy && LLVMGetTypeKind(semanticReturnTy) == LLVMFunctionTypeKind) {
            semanticReturnTy = LLVMPointerType(semanticReturnTy, 0);
        } else if (semanticReturnTy && LLVMGetTypeKind(semanticReturnTy) == LLVMArrayTypeKind) {
            semanticReturnTy = LLVMPointerType(semanticReturnTy, 0);
        }
    }

    bool useCallIndirectSRet = false;
    LLVMValueRef callIndirectSRetSlot = NULL;
    LLVMValueRef* callIndirectSRetArgs = NULL;
    LLVMValueRef* callArgs = finalArgs;
    size_t callArgCount = argCount;
    LLVMTypeRef callType = calleeType;
    bool needsIndirectAggregateReturn =
        semanticReturnTy && cg_should_lower_indirect_aggregate_return(ctx, semanticReturnTy);
    bool calleeAlreadyUsesIndirectSRet = false;
    if (needsIndirectAggregateReturn &&
        LLVMGetTypeKind(calleeType) == LLVMFunctionTypeKind &&
        LLVMGetTypeKind(LLVMGetReturnType(calleeType)) == LLVMVoidTypeKind &&
        LLVMCountParamTypes(calleeType) > 0) {
        LLVMTypeRef firstParamTy = NULL;
        LLVMGetParamTypes(calleeType, &firstParamTy);
        LLVMTypeRef expectedPtrTy = LLVMPointerType(semanticReturnTy, 0);
        calleeAlreadyUsesIndirectSRet = (firstParamTy == expectedPtrTy);
    }

    if (needsIndirectAggregateReturn) {
        callIndirectSRetSlot = cg_build_entry_alloca(ctx, semanticReturnTy, "call.indirect.sret.slot");
        if (callIndirectSRetSlot) {
            LLVMBuildStore(ctx->builder,
                           LLVMConstNull(semanticReturnTy),
                           callIndirectSRetSlot);
            if (calleeAlreadyUsesIndirectSRet) {
                callIndirectSRetArgs = (LLVMValueRef*)calloc(argCount + 1u, sizeof(LLVMValueRef));
                if (callIndirectSRetArgs) {
                    LLVMValueRef sretArg = callIndirectSRetSlot;
                    LLVMTypeRef expectedPtrTy = LLVMPointerType(semanticReturnTy, 0);
                    if (LLVMTypeOf(sretArg) != expectedPtrTy) {
                        sretArg = LLVMBuildBitCast(ctx->builder,
                                                   sretArg,
                                                   expectedPtrTy,
                                                   "call.indirect.sret.arg.cast");
                    }
                    callIndirectSRetArgs[0] = sretArg;
                    for (size_t i = 0; i < argCount; ++i) {
                        callIndirectSRetArgs[i + 1u] = finalArgs ? finalArgs[i] : NULL;
                    }
                    callArgs = callIndirectSRetArgs;
                    callArgCount = argCount + 1u;
                    useCallIndirectSRet = true;
                }
            } else {
                unsigned fixedParamCount = LLVMCountParamTypes(calleeType);
                LLVMTypeRef* fixedParamTypes = NULL;
                LLVMTypeRef* loweredParamTypes = NULL;
                bool isVariadic = LLVMIsFunctionVarArg(calleeType) != 0;
                if (fixedParamCount > 0) {
                    fixedParamTypes = (LLVMTypeRef*)calloc(fixedParamCount, sizeof(LLVMTypeRef));
                    if (fixedParamTypes) {
                        LLVMGetParamTypes(calleeType, fixedParamTypes);
                    }
                }
                loweredParamTypes = (LLVMTypeRef*)calloc(fixedParamCount + 1u, sizeof(LLVMTypeRef));
                if (loweredParamTypes) {
                    loweredParamTypes[0] = LLVMPointerType(semanticReturnTy, 0);
                    for (unsigned i = 0; i < fixedParamCount; ++i) {
                        loweredParamTypes[i + 1u] = fixedParamTypes ? fixedParamTypes[i] : NULL;
                    }
                    callType = LLVMFunctionType(LLVMVoidTypeInContext(ctx->llvmContext),
                                                loweredParamTypes,
                                                fixedParamCount + 1u,
                                                isVariadic ? 1 : 0);
                    function = LLVMBuildBitCast(ctx->builder,
                                                function,
                                                LLVMPointerType(callType, 0),
                                                "call.indirect.sret.cast");
                    callIndirectSRetArgs = (LLVMValueRef*)calloc(argCount + 1u, sizeof(LLVMValueRef));
                    if (callIndirectSRetArgs) {
                        LLVMValueRef sretArg = callIndirectSRetSlot;
                        LLVMTypeRef expectedPtrTy = LLVMPointerType(semanticReturnTy, 0);
                        if (LLVMTypeOf(sretArg) != expectedPtrTy) {
                            sretArg = LLVMBuildBitCast(ctx->builder,
                                                       sretArg,
                                                       expectedPtrTy,
                                                       "call.indirect.sret.arg.cast");
                        }
                        callIndirectSRetArgs[0] = sretArg;
                        for (size_t i = 0; i < argCount; ++i) {
                            callIndirectSRetArgs[i + 1u] = finalArgs ? finalArgs[i] : NULL;
                        }
                        callArgs = callIndirectSRetArgs;
                        callArgCount = argCount + 1u;
                        useCallIndirectSRet = true;
                    }
                }
                free(loweredParamTypes);
                free(fixedParamTypes);
            }
        }
    }

    LLVMTypeRef callRetTy = LLVMGetReturnType(callType);
    bool callReturnsVoid = callRetTy && LLVMGetTypeKind(callRetTy) == LLVMVoidTypeKind;
    LLVMValueRef call = LLVMBuildCall2(ctx->builder,
                                       callType,
                                       function,
                                       callArgs,
                                       callArgCount,
                                       callReturnsVoid ? "" : "calltmp");
    if (useCallIndirectSRet && semanticReturnTy) {
        unsigned sretKind = LLVMGetEnumAttributeKindForName("sret", 4);
        if (sretKind != 0) {
            LLVMAttributeRef sretAttr = LLVMCreateTypeAttribute(ctx->llvmContext,
                                                                sretKind,
                                                                semanticReturnTy);
            LLVMAddCallSiteAttribute(call, 1, sretAttr);
        }
    }
    unsigned fnCC = LLVMGetFunctionCallConv(function);
    if (fnCC != 0) {
        LLVMSetInstructionCallConv(call, fnCC);
    }
    if (externalAbiParamTypes) free(externalAbiParamTypes);
    if (externalAbiArgs) free(externalAbiArgs);
    if (callIndirectSRetArgs) free(callIndirectSRetArgs);
    if (promotedArgs) free(promotedArgs);
    free(args);
    if (callReturnsVoid && !useCallIndirectSRet) {
        CG_CALL_RETURN(NULL);
    }

    LLVMValueRef result = call;
    if (useCallIndirectSRet && callIndirectSRetSlot && semanticReturnTy) {
        result = LLVMBuildLoad2(ctx->builder,
                                semanticReturnTy,
                                callIndirectSRetSlot,
                                "call.indirect.sret.result");
        CG_CALL_RETURN(result);
    }

    if (semanticReturnTy) {
        LLVMTypeKind semanticKind = LLVMGetTypeKind(semanticReturnTy);
        LLVMTypeKind callKind = LLVMGetTypeKind(callRetTy);
        if ((semanticKind == LLVMStructTypeKind || semanticKind == LLVMArrayTypeKind) &&
            (callKind == LLVMIntegerTypeKind || callKind == LLVMArrayTypeKind) &&
            semanticReturnTy != callRetTy) {
            result = cg_unpack_aggregate_from_abi_return(ctx,
                                                         call,
                                                         semanticReturnTy,
                                                         "call.abi.unpack");
        }
    }
    CG_CALL_RETURN(result);
#undef CG_CALL_RETURN
    return NULL;
}
