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

    if (!isLoad && !isStore && !isExchange && !isInit) {
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
                                                           "atomic.exchange.op.cast");
    LLVMAtomicOrdering ordering = LLVMAtomicOrderingSequentiallyConsistent;
    if (node->functionCall.argumentCount >= 3 && args[2]) {
        ordering = cg_atomic_order_from_builtin_arg(args[2],
                                                    LLVMAtomicOrderingSequentiallyConsistent,
                                                    false,
                                                    false);
    }
    LLVMValueRef exchange = LLVMBuildAtomicRMW(ctx->builder,
                                               LLVMAtomicRMWBinOpXchg,
                                               atomicPtr,
                                               opValue,
                                               ordering,
                                               0);
    *resultOut = cg_builtin_atomic_cast_result(ctx,
                                               node,
                                               exchange,
                                               valueType,
                                               atomicType,
                                               "atomic.exchange.result.cast");
    free(args);
    return true;
}
