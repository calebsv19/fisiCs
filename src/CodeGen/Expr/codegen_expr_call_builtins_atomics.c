// SPDX-License-Identifier: Apache-2.0

#include "codegen_expr_internal.h"

#include <stdlib.h>
#include <string.h>

static bool cg_builtin_is_c11_atomic_load(const char* calleeName) {
    return calleeName &&
           (strcmp(calleeName, "__c11_atomic_load") == 0 ||
            strcmp(calleeName, "atomic_load_explicit") == 0);
}

static bool cg_builtin_is_c11_atomic_store(const char* calleeName) {
    return calleeName &&
           (strcmp(calleeName, "__c11_atomic_store") == 0 ||
            strcmp(calleeName, "atomic_store_explicit") == 0);
}

static bool cg_builtin_is_c11_atomic_exchange(const char* calleeName) {
    return calleeName &&
           (strcmp(calleeName, "__c11_atomic_exchange") == 0 ||
            strcmp(calleeName, "atomic_exchange_explicit") == 0);
}

static bool cg_builtin_is_c11_atomic_init(const char* calleeName) {
    return calleeName &&
           (strcmp(calleeName, "__c11_atomic_init") == 0 ||
            strcmp(calleeName, "atomic_init") == 0);
}

static bool cg_builtin_is_c11_atomic_compare_exchange(const char* calleeName, bool* isWeakOut) {
    if (isWeakOut) {
        *isWeakOut = false;
    }
    if (!calleeName) {
        return false;
    }
    if (strcmp(calleeName, "__c11_atomic_compare_exchange_strong") == 0 ||
        strcmp(calleeName, "atomic_compare_exchange_strong_explicit") == 0) {
        return true;
    }
    if (strcmp(calleeName, "__c11_atomic_compare_exchange_weak") == 0 ||
        strcmp(calleeName, "atomic_compare_exchange_weak_explicit") == 0) {
        if (isWeakOut) {
            *isWeakOut = true;
        }
        return true;
    }
    return false;
}

static bool cg_builtin_is_c11_atomic_fence(const char* calleeName, bool* isSignalOut) {
    if (isSignalOut) {
        *isSignalOut = false;
    }
    return calleeName &&
           (strcmp(calleeName, "__c11_atomic_thread_fence") == 0 ||
            strcmp(calleeName, "atomic_thread_fence") == 0 ||
            ((strcmp(calleeName, "__c11_atomic_signal_fence") == 0 ||
              strcmp(calleeName, "atomic_signal_fence") == 0) &&
             ((isSignalOut && (*isSignalOut = true)) || true)));
}

static bool cg_builtin_atomic_fetch_binop(const char* calleeName, LLVMAtomicRMWBinOp* opOut) {
    if (!calleeName || !opOut) return false;
    if (strcmp(calleeName, "__c11_atomic_fetch_add") == 0 ||
        strcmp(calleeName, "atomic_fetch_add_explicit") == 0) {
        *opOut = LLVMAtomicRMWBinOpAdd;
        return true;
    }
    if (strcmp(calleeName, "__c11_atomic_fetch_sub") == 0 ||
        strcmp(calleeName, "atomic_fetch_sub_explicit") == 0) {
        *opOut = LLVMAtomicRMWBinOpSub;
        return true;
    }
    if (strcmp(calleeName, "__c11_atomic_fetch_or") == 0 ||
        strcmp(calleeName, "atomic_fetch_or_explicit") == 0) {
        *opOut = LLVMAtomicRMWBinOpOr;
        return true;
    }
    if (strcmp(calleeName, "__c11_atomic_fetch_xor") == 0 ||
        strcmp(calleeName, "atomic_fetch_xor_explicit") == 0) {
        *opOut = LLVMAtomicRMWBinOpXor;
        return true;
    }
    if (strcmp(calleeName, "__c11_atomic_fetch_and") == 0 ||
        strcmp(calleeName, "atomic_fetch_and_explicit") == 0) {
        *opOut = LLVMAtomicRMWBinOpAnd;
        return true;
    }
    return false;
}

static LLVMTypeRef cg_builtin_atomic_value_type(CodegenContext* ctx,
                                                ASTNode* pointerNode,
                                                LLVMValueRef atomicPtr) {
    const ParsedType* ptrParsed = cg_resolve_expression_type(ctx, pointerNode);
    LLVMTypeRef valueType = cg_element_type_from_pointer(ctx, ptrParsed, LLVMTypeOf(atomicPtr));
    if (!valueType || LLVMGetTypeKind(valueType) == LLVMVoidTypeKind) {
        valueType = LLVMInt8TypeInContext(ctx->llvmContext);
    }
    return valueType;
}

static LLVMTypeRef cg_builtin_atomic_storage_type(CodegenContext* ctx, LLVMTypeRef valueType) {
    LLVMTypeRef atomicType = valueType;
    if (LLVMGetTypeKind(atomicType) == LLVMIntegerTypeKind &&
        LLVMGetIntTypeWidth(atomicType) == 1) {
        atomicType = LLVMInt8TypeInContext(ctx->llvmContext);
    }
    return atomicType;
}

static LLVMValueRef cg_builtin_atomic_cast_op_value(CodegenContext* ctx,
                                                    LLVMValueRef value,
                                                    LLVMTypeRef valueType,
                                                    LLVMTypeRef atomicType,
                                                    const char* castName) {
    if (atomicType == valueType) {
        return value;
    }
    if (LLVMGetTypeKind(atomicType) == LLVMIntegerTypeKind &&
        LLVMGetTypeKind(valueType) == LLVMIntegerTypeKind) {
        return LLVMBuildIntCast2(ctx->builder, value, atomicType, false, castName);
    }
    return cg_cast_value(ctx, value, atomicType, NULL, NULL, castName);
}

static LLVMValueRef cg_builtin_atomic_cast_result(CodegenContext* ctx,
                                                  ASTNode* node,
                                                  LLVMValueRef value,
                                                  LLVMTypeRef valueType,
                                                  LLVMTypeRef atomicType,
                                                  const char* castName) {
    LLVMValueRef typedValue = value;
    if (atomicType != valueType) {
        if (LLVMGetTypeKind(atomicType) == LLVMIntegerTypeKind &&
            LLVMGetTypeKind(valueType) == LLVMIntegerTypeKind) {
            typedValue = LLVMBuildIntCast2(ctx->builder, typedValue, valueType, false, castName);
        } else {
            typedValue = cg_cast_value(ctx, typedValue, valueType, NULL, NULL, castName);
        }
    }
    return cg_atomic_cast_call_result(ctx, node, typedValue, castName);
}

bool cg_try_codegen_atomic_builtin_call(CodegenContext* ctx,
                                        ASTNode* node,
                                        const char* calleeName,
                                        LLVMValueRef* args,
                                        LLVMValueRef* resultOut) {
    bool isLoad = cg_builtin_is_c11_atomic_load(calleeName);
    bool isStore = cg_builtin_is_c11_atomic_store(calleeName);
    bool isExchange = cg_builtin_is_c11_atomic_exchange(calleeName);
    bool isInit = cg_builtin_is_c11_atomic_init(calleeName);
    bool isWeakCompareExchange = false;
    bool isCompareExchange =
        cg_builtin_is_c11_atomic_compare_exchange(calleeName, &isWeakCompareExchange);
    bool isSignalFence = false;
    bool isFence = cg_builtin_is_c11_atomic_fence(calleeName, &isSignalFence);
    LLVMAtomicRMWBinOp fetchBinop = LLVMAtomicRMWBinOpAdd;
    bool isFetch = cg_builtin_atomic_fetch_binop(calleeName, &fetchBinop);

    if (!isLoad && !isStore && !isExchange && !isInit && !isCompareExchange && !isFence && !isFetch) {
        return false;
    }

    if (!ctx || !node || !resultOut) {
        return true;
    }

    if (isLoad) {
        if (node->functionCall.argumentCount < 1 || !args || !args[0]) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMValueRef atomicPtr = args[0];
        if (LLVMGetTypeKind(LLVMTypeOf(atomicPtr)) != LLVMPointerTypeKind) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMTypeRef valueType =
            cg_builtin_atomic_value_type(ctx, node->functionCall.arguments[0], atomicPtr);
        LLVMTypeRef atomicType = cg_builtin_atomic_storage_type(ctx, valueType);
        atomicPtr = cg_atomic_cast_pointer(ctx, atomicPtr, atomicType, "atomic.load.ptr.cast");
        if (!atomicPtr) {
            free(args);
            *resultOut = NULL;
            return true;
        }

        LLVMValueRef load = LLVMBuildLoad2(ctx->builder, atomicType, atomicPtr, "atomic.load");
        LLVMAtomicOrdering ordering = LLVMAtomicOrderingSequentiallyConsistent;
        if (node->functionCall.argumentCount >= 2 && args[1]) {
            ordering = cg_atomic_order_from_builtin_arg(args[1],
                                                        LLVMAtomicOrderingSequentiallyConsistent,
                                                        true,
                                                        false);
        }
        LLVMSetOrdering(load, ordering);
        *resultOut = cg_builtin_atomic_cast_result(ctx,
                                                   node,
                                                   load,
                                                   valueType,
                                                   atomicType,
                                                   "atomic.load.result.cast");
        free(args);
        return true;
    }

    if (isStore || isInit) {
        if (node->functionCall.argumentCount < 2 || !args || !args[0] || !args[1]) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMValueRef atomicPtr = args[0];
        if (LLVMGetTypeKind(LLVMTypeOf(atomicPtr)) != LLVMPointerTypeKind) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMTypeRef valueType =
            cg_builtin_atomic_value_type(ctx, node->functionCall.arguments[0], atomicPtr);
        LLVMTypeRef atomicType = cg_builtin_atomic_storage_type(ctx, valueType);
        atomicPtr = cg_atomic_cast_pointer(ctx, atomicPtr, atomicType, "atomic.store.ptr.cast");
        if (!atomicPtr) {
            free(args);
            *resultOut = NULL;
            return true;
        }

        LLVMValueRef desired = cg_atomic_cast_value(ctx,
                                                    args[1],
                                                    valueType,
                                                    node->functionCall.arguments[1],
                                                    "atomic.store.value.cast");
        if (!desired) {
            free(args);
            *resultOut = NULL;
            return true;
        }

        LLVMValueRef opValue = cg_builtin_atomic_cast_op_value(ctx,
                                                               desired,
                                                               valueType,
                                                               atomicType,
                                                               "atomic.store.op.cast");
        if (isStore) {
            LLVMAtomicOrdering ordering = LLVMAtomicOrderingSequentiallyConsistent;
            if (node->functionCall.argumentCount >= 3 && args[2]) {
                ordering = cg_atomic_order_from_builtin_arg(args[2],
                                                            LLVMAtomicOrderingSequentiallyConsistent,
                                                            false,
                                                            true);
            }
            LLVMValueRef store = LLVMBuildStore(ctx->builder, opValue, atomicPtr);
            LLVMSetOrdering(store, ordering);
        } else {
            (void)LLVMBuildStore(ctx->builder, opValue, atomicPtr);
        }
        free(args);
        *resultOut = NULL;
        return true;
    }

    if (isFence) {
        LLVMAtomicOrdering ordering = LLVMAtomicOrderingSequentiallyConsistent;
        if (node->functionCall.argumentCount >= 1 && args && args[0]) {
            ordering = cg_atomic_order_from_builtin_arg(args[0],
                                                        LLVMAtomicOrderingSequentiallyConsistent,
                                                        false,
                                                        false);
        }
        if (ordering != LLVMAtomicOrderingMonotonic &&
            ordering != LLVMAtomicOrderingNotAtomic &&
            ordering != LLVMAtomicOrderingUnordered) {
            (void)LLVMBuildFence(ctx->builder,
                                 ordering,
                                 isSignalFence ? 1 : 0,
                                 isSignalFence ? "atomic.signal.fence" : "atomic.thread.fence");
        }
        free(args);
        *resultOut = NULL;
        return true;
    }

    if (isCompareExchange) {
        if (node->functionCall.argumentCount < 3 || !args || !args[0] || !args[1] || !args[2]) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMValueRef atomicPtr = args[0];
        LLVMValueRef expectedPtr = args[1];
        if (LLVMGetTypeKind(LLVMTypeOf(atomicPtr)) != LLVMPointerTypeKind ||
            LLVMGetTypeKind(LLVMTypeOf(expectedPtr)) != LLVMPointerTypeKind) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMTypeRef valueType =
            cg_builtin_atomic_value_type(ctx, node->functionCall.arguments[0], atomicPtr);
        LLVMTypeRef atomicType = cg_builtin_atomic_storage_type(ctx, valueType);
        atomicPtr = cg_atomic_cast_pointer(ctx, atomicPtr, atomicType, "atomic.cmpxchg.ptr.cast");
        expectedPtr = cg_atomic_cast_pointer(ctx, expectedPtr, atomicType, "atomic.cmpxchg.expected.ptr.cast");
        if (!atomicPtr || !expectedPtr) {
            free(args);
            *resultOut = NULL;
            return true;
        }

        LLVMValueRef expected = LLVMBuildLoad2(ctx->builder,
                                               atomicType,
                                               expectedPtr,
                                               "atomic.cmpxchg.expected");
        LLVMValueRef desired = cg_atomic_cast_value(ctx,
                                                    args[2],
                                                    valueType,
                                                    node->functionCall.arguments[2],
                                                    "atomic.cmpxchg.desired.value.cast");
        if (!desired) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMValueRef opValue = cg_builtin_atomic_cast_op_value(ctx,
                                                               desired,
                                                               valueType,
                                                               atomicType,
                                                               "atomic.cmpxchg.desired.op.cast");
        LLVMAtomicOrdering successOrdering = LLVMAtomicOrderingSequentiallyConsistent;
        if (node->functionCall.argumentCount >= 4 && args[3]) {
            successOrdering = cg_atomic_order_from_builtin_arg(args[3],
                                                               LLVMAtomicOrderingSequentiallyConsistent,
                                                               false,
                                                               false);
        }
        LLVMAtomicOrdering failureOrdering = LLVMAtomicOrderingSequentiallyConsistent;
        if (node->functionCall.argumentCount >= 5 && args[4]) {
            failureOrdering = cg_atomic_order_from_builtin_arg(args[4],
                                                               LLVMAtomicOrderingSequentiallyConsistent,
                                                               true,
                                                               false);
        }
        if (failureOrdering == LLVMAtomicOrderingRelease ||
            failureOrdering == LLVMAtomicOrderingAcquireRelease) {
            failureOrdering = LLVMAtomicOrderingAcquire;
        }
        LLVMValueRef cmpxchg = LLVMBuildAtomicCmpXchg(ctx->builder,
                                                      atomicPtr,
                                                      expected,
                                                      opValue,
                                                      successOrdering,
                                                      failureOrdering,
                                                      0);
        LLVMSetWeak(cmpxchg, isWeakCompareExchange ? 1 : 0);
        LLVMValueRef observed = LLVMBuildExtractValue(ctx->builder,
                                                      cmpxchg,
                                                      0,
                                                      "atomic.cmpxchg.observed");
        LLVMValueRef success = LLVMBuildExtractValue(ctx->builder,
                                                     cmpxchg,
                                                     1,
                                                     "atomic.cmpxchg.success");

        LLVMBasicBlockRef currentBlock = LLVMGetInsertBlock(ctx->builder);
        LLVMValueRef function = LLVMGetBasicBlockParent(currentBlock);
        LLVMBasicBlockRef failBlock =
            LLVMAppendBasicBlockInContext(ctx->llvmContext, function, "atomic.cmpxchg.fail");
        LLVMBasicBlockRef contBlock =
            LLVMAppendBasicBlockInContext(ctx->llvmContext, function, "atomic.cmpxchg.cont");
        LLVMBuildCondBr(ctx->builder, success, contBlock, failBlock);

        LLVMPositionBuilderAtEnd(ctx->builder, failBlock);
        (void)LLVMBuildStore(ctx->builder, observed, expectedPtr);
        LLVMBuildBr(ctx->builder, contBlock);

        LLVMPositionBuilderAtEnd(ctx->builder, contBlock);
        *resultOut = cg_atomic_cast_call_result(ctx, node, success, "atomic.cmpxchg.result.cast");
        free(args);
        return true;
    }

    if (node->functionCall.argumentCount < 2 || !args || !args[0] || !args[1]) {
        free(args);
        *resultOut = NULL;
        return true;
    }
    LLVMValueRef atomicPtr = args[0];
    if (LLVMGetTypeKind(LLVMTypeOf(atomicPtr)) != LLVMPointerTypeKind) {
        free(args);
        *resultOut = NULL;
        return true;
    }
    LLVMTypeRef valueType =
        cg_builtin_atomic_value_type(ctx, node->functionCall.arguments[0], atomicPtr);
    LLVMTypeRef atomicType = cg_builtin_atomic_storage_type(ctx, valueType);
    atomicPtr = cg_atomic_cast_pointer(ctx, atomicPtr, atomicType, "atomic.exchange.ptr.cast");
    if (!atomicPtr) {
        free(args);
        *resultOut = NULL;
        return true;
    }

    LLVMValueRef desired = cg_atomic_cast_value(ctx,
                                                args[1],
                                                valueType,
                                                node->functionCall.arguments[1],
                                                "atomic.exchange.value.cast");
    if (!desired) {
        free(args);
        *resultOut = NULL;
        return true;
    }

    LLVMValueRef opValue = cg_builtin_atomic_cast_op_value(ctx,
                                                           desired,
                                                           valueType,
                                                           atomicType,
                                                           isFetch ? "atomic.fetch.op.cast" : "atomic.exchange.op.cast");
    LLVMAtomicOrdering ordering = LLVMAtomicOrderingSequentiallyConsistent;
    if (node->functionCall.argumentCount >= 3 && args[2]) {
        ordering = cg_atomic_order_from_builtin_arg(args[2],
                                                    LLVMAtomicOrderingSequentiallyConsistent,
                                                    false,
                                                    false);
    }
    LLVMValueRef exchange = LLVMBuildAtomicRMW(ctx->builder,
                                               isFetch ? fetchBinop : LLVMAtomicRMWBinOpXchg,
                                               atomicPtr,
                                               opValue,
                                               ordering,
                                               0);
    *resultOut = cg_builtin_atomic_cast_result(ctx,
                                               node,
                                               exchange,
                                               valueType,
                                               atomicType,
                                               isFetch ? "atomic.fetch.result.cast" : "atomic.exchange.result.cast");
    free(args);
    return true;
}
