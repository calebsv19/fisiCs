// SPDX-License-Identifier: Apache-2.0

#include "codegen_expr_internal.h"

#include <stdlib.h>

unsigned cg_default_int_bits(CodegenContext* ctx) {
    const TargetLayout* tl = cg_context_get_target_layout(ctx);
    return tl ? (unsigned)tl->intBits : 32;
}

LLVMValueRef cg_apply_default_promotion(CodegenContext* ctx,
                                        LLVMValueRef arg,
                                        ASTNode* argNode) {
    if (!ctx || !arg) return arg;
    LLVMTypeRef ty = LLVMTypeOf(arg);
    if (!ty) return arg;

    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    LLVMTypeKind kind = LLVMGetTypeKind(ty);
    if (kind == LLVMFloatTypeKind) {
        return LLVMBuildFPExt(ctx->builder, arg, LLVMDoubleTypeInContext(llvmCtx), "vararg.fp.promote");
    }

    if (kind == LLVMIntegerTypeKind) {
        unsigned bits = LLVMGetIntTypeWidth(ty);
        unsigned targetBits = cg_default_int_bits(ctx);
        if (bits < targetBits) {
            bool isUnsigned = cg_expression_is_unsigned(ctx, argNode);
            LLVMTypeRef dest = LLVMIntTypeInContext(llvmCtx, targetBits);
            return LLVMBuildIntCast2(ctx->builder, arg, dest, !isUnsigned, "vararg.int.promote");
        }
    }

    return arg;
}

LLVMValueRef cg_prepare_call_argument(CodegenContext* ctx,
                                      LLVMValueRef arg,
                                      LLVMTypeRef paramTy,
                                      const ParsedType* fromParsed,
                                      const ParsedType* toParsed,
                                      const char* castName) {
    if (!ctx || !arg || !paramTy) return arg;
    if (LLVMGetTypeKind(paramTy) == LLVMVoidTypeKind) return arg;

    LLVMTypeRef argTy = LLVMTypeOf(arg);
    if (!argTy || argTy == paramTy) {
        return arg;
    }

    if (LLVMGetTypeKind(paramTy) == LLVMPointerTypeKind) {
        LLVMTypeKind argKind = LLVMGetTypeKind(argTy);
        bool argIsAggregate = (argKind == LLVMStructTypeKind || argKind == LLVMArrayTypeKind);
        bool declaredPointer = toParsed && cg_parsed_type_is_pointerish(toParsed);
        if (argIsAggregate && !declaredPointer) {
            LLVMValueRef tmp = cg_build_entry_alloca(ctx, argTy, "call.arg.indirect.tmp");
            if (tmp) {
                LLVMBuildStore(ctx->builder, arg, tmp);
                return tmp;
            }
        }
    }

    return cg_cast_value(ctx,
                         arg,
                         paramTy,
                         fromParsed,
                         toParsed,
                         castName ? castName : "call.arg.cast");
}

LLVMValueRef cg_emit_va_intrinsic(CodegenContext* ctx,
                                  const char* intrinsicName,
                                  LLVMValueRef listValue) {
    if (!ctx || !intrinsicName || !listValue) return NULL;
    LLVMTypeRef listTy = LLVMTypeOf(listValue);
    if (!listTy || LLVMGetTypeKind(listTy) != LLVMPointerTypeKind) {
        return NULL;
    }
    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    LLVMTypeRef i8Ptr = LLVMPointerType(LLVMInt8TypeInContext(llvmCtx), 0);
    LLVMValueRef casted = LLVMBuildBitCast(ctx->builder, listValue, i8Ptr, "va.list.cast");

    LLVMTypeRef fnTy = LLVMFunctionType(LLVMVoidTypeInContext(llvmCtx), &i8Ptr, 1, 0);
    LLVMValueRef fn = LLVMGetNamedFunction(ctx->module, intrinsicName);
    if (!fn) {
        fn = LLVMAddFunction(ctx->module, intrinsicName, fnTy);
    }
    (void)LLVMBuildCall2(ctx->builder, fnTy, fn, &casted, 1, "");
    return NULL;
}

LLVMValueRef cg_emit_rewritten_builtin_call(CodegenContext* ctx,
                                            const char* targetName,
                                            LLVMTypeRef returnType,
                                            LLVMValueRef* sourceArgs,
                                            size_t sourceCount,
                                            const size_t* keepIndices,
                                            size_t keepCount,
                                            unsigned fixedParamCount,
                                            const LLVMTypeRef* fixedParamTypes,
                                            const bool* fixedParamSigned,
                                            bool isVariadic,
                                            const char* callName) {
    if (!ctx || !targetName || !returnType || !sourceArgs || !keepIndices || keepCount == 0) {
        return NULL;
    }

    if (fixedParamCount > keepCount) {
        fixedParamCount = (unsigned)keepCount;
    }

    LLVMValueRef* callArgs = (LLVMValueRef*)calloc(keepCount, sizeof(LLVMValueRef));
    if (!callArgs) {
        return NULL;
    }
    for (size_t i = 0; i < keepCount; ++i) {
        size_t idx = keepIndices[i];
        if (idx >= sourceCount || !sourceArgs[idx]) {
            free(callArgs);
            return NULL;
        }
        callArgs[i] = sourceArgs[idx];
        if (i < fixedParamCount && fixedParamTypes && fixedParamTypes[i] && LLVMTypeOf(callArgs[i]) != fixedParamTypes[i]) {
            LLVMTypeRef fromTy = LLVMTypeOf(callArgs[i]);
            LLVMTypeRef toTy = fixedParamTypes[i];
            LLVMTypeKind fromKind = fromTy ? LLVMGetTypeKind(fromTy) : LLVMVoidTypeKind;
            LLVMTypeKind toKind = LLVMGetTypeKind(toTy);
            if (fromKind == LLVMPointerTypeKind && toKind == LLVMPointerTypeKind) {
                callArgs[i] = LLVMBuildBitCast(ctx->builder, callArgs[i], toTy, "builtin.arg.ptrcast");
            } else if (fromKind == LLVMIntegerTypeKind && toKind == LLVMIntegerTypeKind) {
                unsigned fromBits = LLVMGetIntTypeWidth(fromTy);
                unsigned toBits = LLVMGetIntTypeWidth(toTy);
                if (fromBits != toBits) {
                    bool isSigned = fixedParamSigned ? fixedParamSigned[i] : false;
                    callArgs[i] = LLVMBuildIntCast2(ctx->builder, callArgs[i], toTy, isSigned, "builtin.arg.intcast");
                }
            } else if (fromKind == LLVMPointerTypeKind && toKind == LLVMIntegerTypeKind) {
                callArgs[i] = LLVMBuildPtrToInt(ctx->builder, callArgs[i], toTy, "builtin.arg.ptrtoint");
            } else if (fromKind == LLVMIntegerTypeKind && toKind == LLVMPointerTypeKind) {
                callArgs[i] = LLVMBuildIntToPtr(ctx->builder, callArgs[i], toTy, "builtin.arg.inttoptr");
            } else {
                free(callArgs);
                return NULL;
            }
        }
    }

    LLVMTypeRef* loweredParamTypes = NULL;
    if (fixedParamCount > 0) {
        LLVMTypeRef* paramTypes = (LLVMTypeRef*)calloc(fixedParamCount, sizeof(LLVMTypeRef));
        if (!paramTypes) {
            free(callArgs);
            return NULL;
        }
        for (unsigned i = 0; i < fixedParamCount; ++i) {
            if (fixedParamTypes && fixedParamTypes[i]) {
                paramTypes[i] = fixedParamTypes[i];
            } else {
                paramTypes[i] = LLVMTypeOf(callArgs[i]);
            }
            if (!paramTypes[i]) {
                paramTypes[i] = LLVMInt32TypeInContext(ctx->llvmContext);
            }
        }
        loweredParamTypes = paramTypes;
    }

    LLVMTypeRef requestedFnTy = LLVMFunctionType(returnType,
                                                 loweredParamTypes,
                                                 fixedParamCount,
                                                 isVariadic ? 1 : 0);
    free(loweredParamTypes);

    const Symbol* targetSym = cg_lookup_global_function_symbol_by_name(ctx, targetName);
    LLVMTypeRef symbolFnTy = cg_function_type_from_symbol(ctx, targetSym);
    if (symbolFnTy && LLVMGetTypeKind(symbolFnTy) == LLVMFunctionTypeKind) {
        requestedFnTy = symbolFnTy;
    }

    LLVMValueRef fn = LLVMGetNamedFunction(ctx->module, targetName);
    LLVMTypeRef fnTy = requestedFnTy;
    if (!fn) {
        fn = LLVMAddFunction(ctx->module, targetName, requestedFnTy);
    } else {
        LLVMTypeRef existingFnTy = LLVMGlobalGetValueType(fn);
        if (!existingFnTy || LLVMGetTypeKind(existingFnTy) != LLVMFunctionTypeKind) {
            LLVMTypeRef fnPtrTy = LLVMTypeOf(fn);
            if (fnPtrTy && LLVMGetTypeKind(fnPtrTy) == LLVMPointerTypeKind) {
                LLVMTypeRef derefTy = cg_dereference_ptr_type(ctx, fnPtrTy, targetName);
                if (derefTy && LLVMGetTypeKind(derefTy) == LLVMFunctionTypeKind) {
                    existingFnTy = derefTy;
                }
            }
        }
        if (existingFnTy && LLVMGetTypeKind(existingFnTy) == LLVMFunctionTypeKind) {
            unsigned existingFixedCount = LLVMCountParamTypes(existingFnTy);
            bool existingVariadic = LLVMIsFunctionVarArg(existingFnTy) != 0;
            if (existingFixedCount > keepCount ||
                (!existingVariadic && keepCount != existingFixedCount)) {
                free(callArgs);
                return NULL;
            }
            if (existingFixedCount > 0) {
                LLVMTypeRef* existingParamTypes =
                    (LLVMTypeRef*)calloc(existingFixedCount, sizeof(LLVMTypeRef));
                if (!existingParamTypes) {
                    free(callArgs);
                    return NULL;
                }
                LLVMGetParamTypes(existingFnTy, existingParamTypes);
                for (unsigned i = 0; i < existingFixedCount; ++i) {
                    LLVMTypeRef toTy = existingParamTypes[i];
                    LLVMValueRef arg = callArgs[i];
                    if (!toTy || !arg) {
                        free(existingParamTypes);
                        free(callArgs);
                        return NULL;
                    }
                    if (LLVMTypeOf(arg) == toTy) continue;
                    LLVMTypeRef fromTy = LLVMTypeOf(arg);
                    LLVMTypeKind fromKind = fromTy ? LLVMGetTypeKind(fromTy) : LLVMVoidTypeKind;
                    LLVMTypeKind toKind = LLVMGetTypeKind(toTy);
                    if (fromKind == LLVMPointerTypeKind && toKind == LLVMPointerTypeKind) {
                        callArgs[i] = LLVMBuildBitCast(ctx->builder, arg, toTy, "builtin.arg.ptrcast.existing");
                    } else if (fromKind == LLVMIntegerTypeKind && toKind == LLVMIntegerTypeKind) {
                        unsigned fromBits = LLVMGetIntTypeWidth(fromTy);
                        unsigned toBits = LLVMGetIntTypeWidth(toTy);
                        if (fromBits != toBits) {
                            callArgs[i] = LLVMBuildIntCast2(ctx->builder,
                                                            arg,
                                                            toTy,
                                                            false,
                                                            "builtin.arg.intcast.existing");
                        }
                    } else if (fromKind == LLVMPointerTypeKind && toKind == LLVMIntegerTypeKind) {
                        callArgs[i] = LLVMBuildPtrToInt(ctx->builder, arg, toTy, "builtin.arg.ptrtoint.existing");
                    } else if (fromKind == LLVMIntegerTypeKind && toKind == LLVMPointerTypeKind) {
                        callArgs[i] = LLVMBuildIntToPtr(ctx->builder, arg, toTy, "builtin.arg.inttoptr.existing");
                    } else {
                        free(existingParamTypes);
                        free(callArgs);
                        return NULL;
                    }
                }
                free(existingParamTypes);
            }
            fnTy = existingFnTy;
        }
    }

    if (fnTy && LLVMGetTypeKind(fnTy) == LLVMFunctionTypeKind) {
        unsigned emitFixedCount = LLVMCountParamTypes(fnTy);
        if (emitFixedCount > 0) {
            LLVMTypeRef* emitParamTypes = (LLVMTypeRef*)calloc(emitFixedCount, sizeof(LLVMTypeRef));
            if (!emitParamTypes) {
                free(callArgs);
                return NULL;
            }
            LLVMGetParamTypes(fnTy, emitParamTypes);
            size_t castCount = keepCount < emitFixedCount ? keepCount : (size_t)emitFixedCount;
            for (size_t i = 0; i < castCount; ++i) {
                LLVMTypeRef toTy = emitParamTypes[i];
                LLVMValueRef arg = callArgs[i];
                if (!toTy || !arg) {
                    free(emitParamTypes);
                    free(callArgs);
                    return NULL;
                }
                if (LLVMTypeOf(arg) == toTy) continue;
                LLVMTypeRef fromTy = LLVMTypeOf(arg);
                LLVMTypeKind fromKind = fromTy ? LLVMGetTypeKind(fromTy) : LLVMVoidTypeKind;
                LLVMTypeKind toKind = LLVMGetTypeKind(toTy);
                if (fromKind == LLVMPointerTypeKind && toKind == LLVMPointerTypeKind) {
                    callArgs[i] = LLVMBuildBitCast(ctx->builder, arg, toTy, "builtin.arg.ptrcast.emit");
                } else if (fromKind == LLVMIntegerTypeKind && toKind == LLVMIntegerTypeKind) {
                    unsigned fromBits = LLVMGetIntTypeWidth(fromTy);
                    unsigned toBits = LLVMGetIntTypeWidth(toTy);
                    if (fromBits != toBits) {
                        callArgs[i] = LLVMBuildIntCast2(ctx->builder,
                                                        arg,
                                                        toTy,
                                                        false,
                                                        "builtin.arg.intcast.emit");
                    }
                } else if (fromKind == LLVMPointerTypeKind && toKind == LLVMIntegerTypeKind) {
                    callArgs[i] = LLVMBuildPtrToInt(ctx->builder, arg, toTy, "builtin.arg.ptrtoint.emit");
                } else if (fromKind == LLVMIntegerTypeKind && toKind == LLVMPointerTypeKind) {
                    callArgs[i] = LLVMBuildIntToPtr(ctx->builder, arg, toTy, "builtin.arg.inttoptr.emit");
                } else {
                    free(emitParamTypes);
                    free(callArgs);
                    return NULL;
                }
            }
            free(emitParamTypes);
        }
    }

    bool requestedReturnsVoid = LLVMGetTypeKind(returnType) == LLVMVoidTypeKind;
    LLVMTypeRef callReturnTy = LLVMGetReturnType(fnTy);
    bool callReturnsVoid = LLVMGetTypeKind(callReturnTy) == LLVMVoidTypeKind;
    const char* emittedName = callReturnsVoid ? "" : (callName ? callName : "builtin.call");
    LLVMValueRef call = LLVMBuildCall2(ctx->builder,
                                       fnTy,
                                       fn,
                                       callArgs,
                                       (unsigned)keepCount,
                                       emittedName);
    free(callArgs);
    if (callReturnsVoid || !call) {
        return requestedReturnsVoid ? NULL : call;
    }
    if (callReturnTy == returnType || requestedReturnsVoid) {
        return call;
    }

    LLVMTypeKind fromKind = LLVMGetTypeKind(callReturnTy);
    LLVMTypeKind toKind = LLVMGetTypeKind(returnType);
    if (fromKind == LLVMPointerTypeKind && toKind == LLVMPointerTypeKind) {
        return LLVMBuildBitCast(ctx->builder, call, returnType, "builtin.ret.ptrcast");
    }
    if (fromKind == LLVMIntegerTypeKind && toKind == LLVMIntegerTypeKind) {
        unsigned fromBits = LLVMGetIntTypeWidth(callReturnTy);
        unsigned toBits = LLVMGetIntTypeWidth(returnType);
        if (fromBits == toBits) return call;
        return LLVMBuildIntCast2(ctx->builder, call, returnType, false, "builtin.ret.intcast");
    }
    if (fromKind == LLVMPointerTypeKind && toKind == LLVMIntegerTypeKind) {
        return LLVMBuildPtrToInt(ctx->builder, call, returnType, "builtin.ret.ptrtoint");
    }
    if (fromKind == LLVMIntegerTypeKind && toKind == LLVMPointerTypeKind) {
        return LLVMBuildIntToPtr(ctx->builder, call, returnType, "builtin.ret.inttoptr");
    }
    return call;
}
