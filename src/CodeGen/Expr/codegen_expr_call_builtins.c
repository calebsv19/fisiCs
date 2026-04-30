// SPDX-License-Identifier: Apache-2.0

#include "codegen_expr_internal.h"

#include "AST/ast_node.h"
#include "Extensions/Units/units_conversion.h"
#include "Extensions/extension_units_expr_table.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool isFisicsUnitConvertBuiltin(const char* calleeName) {
    return calleeName &&
           (strcmp(calleeName, "fisics_convert_unit") == 0 ||
            strcmp(calleeName, "__builtin_fisics_convert_unit") == 0);
}

static const char* builtinStringLiteralPayload(const ASTNode* node) {
    if (!node || node->type != AST_STRING_LITERAL) return NULL;
    const char* payload = NULL;
    (void)ast_literal_encoding(node->valueNode.value, &payload);
    return payload;
}

bool cg_try_codegen_builtin_call(CodegenContext* ctx,
                                 ASTNode* node,
                                 const char* calleeName,
                                 LLVMValueRef* args,
                                 LLVMTypeRef i8PtrType,
                                 LLVMTypeRef sizeType,
                                 LLVMTypeRef intType,
                                 LLVMValueRef* resultOut) {
    if (!ctx || !node || !resultOut || !calleeName) {
        return false;
    }

    bool isVaStartBuiltin =
        strcmp(calleeName, "va_start") == 0 || strcmp(calleeName, "__builtin_va_start") == 0;
    bool isVaArgBuiltin =
        strcmp(calleeName, "__builtin_va_arg") == 0 || strcmp(calleeName, "va_arg") == 0;
    bool isVaEndBuiltin =
        strcmp(calleeName, "va_end") == 0 || strcmp(calleeName, "__builtin_va_end") == 0;
    bool isVaCopyBuiltin =
        strcmp(calleeName, "__builtin_va_copy") == 0 || strcmp(calleeName, "va_copy") == 0;
    bool isAllocaBuiltin = strcmp(calleeName, "__builtin_alloca") == 0;
    bool isObjectSizeBuiltin = strcmp(calleeName, "__builtin_object_size") == 0;
    bool isFabsfBuiltin = strcmp(calleeName, "__builtin_fabsf") == 0;
    bool isFabsBuiltin = strcmp(calleeName, "__builtin_fabs") == 0;
    bool isFabslBuiltin = strcmp(calleeName, "__builtin_fabsl") == 0;
    bool isInffBuiltin = strcmp(calleeName, "__builtin_inff") == 0;
    bool isInfBuiltin = strcmp(calleeName, "__builtin_inf") == 0;
    bool isInflBuiltin = strcmp(calleeName, "__builtin_infl") == 0;
    bool isNanfBuiltin = strcmp(calleeName, "__builtin_nanf") == 0;
    bool isNanBuiltin = strcmp(calleeName, "__builtin_nan") == 0;
    bool isNanlBuiltin = strcmp(calleeName, "__builtin_nanl") == 0;
    bool isHugeValfBuiltin = strcmp(calleeName, "__builtin_huge_valf") == 0;
    bool isHugeValBuiltin = strcmp(calleeName, "__builtin_huge_val") == 0;
    bool isHugeVallBuiltin = strcmp(calleeName, "__builtin_huge_vall") == 0;
    bool isSnprintfChkBuiltin = strcmp(calleeName, "__builtin___snprintf_chk") == 0;
    bool isVsnprintfChkBuiltin = strcmp(calleeName, "__builtin___vsnprintf_chk") == 0;
    bool isSprintfChkBuiltin = strcmp(calleeName, "__builtin___sprintf_chk") == 0;
    bool isMemcpyChkBuiltin = strcmp(calleeName, "__builtin___memcpy_chk") == 0;
    bool isMemsetChkBuiltin = strcmp(calleeName, "__builtin___memset_chk") == 0;
    bool isBzeroBuiltin = strcmp(calleeName, "__builtin_bzero") == 0;
    bool isStrncpyChkBuiltin = strcmp(calleeName, "__builtin___strncpy_chk") == 0;
    bool isStrncatChkBuiltin = strcmp(calleeName, "__builtin___strncat_chk") == 0;
    bool isStrcpyChkBuiltin = strcmp(calleeName, "__builtin___strcpy_chk") == 0;
    bool isStrcatChkBuiltin = strcmp(calleeName, "__builtin___strcat_chk") == 0;
    bool isMemmoveChkBuiltin = strcmp(calleeName, "__builtin___memmove_chk") == 0;
    bool isC11AtomicLoadBuiltin =
        strcmp(calleeName, "__c11_atomic_load") == 0 ||
        strcmp(calleeName, "atomic_load_explicit") == 0;
    bool isC11AtomicStoreBuiltin =
        strcmp(calleeName, "__c11_atomic_store") == 0 ||
        strcmp(calleeName, "atomic_store_explicit") == 0;
    bool isC11AtomicExchangeBuiltin =
        strcmp(calleeName, "__c11_atomic_exchange") == 0 ||
        strcmp(calleeName, "atomic_exchange_explicit") == 0;
    bool isC11AtomicInitBuiltin =
        strcmp(calleeName, "__c11_atomic_init") == 0 ||
        strcmp(calleeName, "atomic_init") == 0;
    bool isExpectBuiltin = strcmp(calleeName, "__builtin_expect") == 0;
    bool isFisicsConvertBuiltin = isFisicsUnitConvertBuiltin(calleeName);

    if (!isVaStartBuiltin && !isVaArgBuiltin && !isVaEndBuiltin &&
        !isVaCopyBuiltin && !isAllocaBuiltin && !isObjectSizeBuiltin &&
        !isFabsfBuiltin && !isFabsBuiltin && !isFabslBuiltin &&
        !isInffBuiltin && !isInfBuiltin && !isInflBuiltin &&
        !isNanfBuiltin && !isNanBuiltin && !isNanlBuiltin &&
        !isHugeValfBuiltin && !isHugeValBuiltin && !isHugeVallBuiltin &&
        !isSnprintfChkBuiltin && !isVsnprintfChkBuiltin && !isSprintfChkBuiltin &&
        !isMemcpyChkBuiltin && !isMemsetChkBuiltin && !isBzeroBuiltin &&
        !isStrncpyChkBuiltin && !isStrncatChkBuiltin && !isStrcpyChkBuiltin &&
        !isStrcatChkBuiltin && !isMemmoveChkBuiltin &&
        !isC11AtomicLoadBuiltin && !isC11AtomicStoreBuiltin &&
        !isC11AtomicExchangeBuiltin && !isC11AtomicInitBuiltin &&
        !isExpectBuiltin && !isFisicsConvertBuiltin) {
        return false;
    }

    if (isFisicsConvertBuiltin) {
        if (node->functionCall.argumentCount < 2 || !args || !args[0]) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMValueRef sourceValue = args[0];
        ASTNode* sourceNode = node->functionCall.arguments ? node->functionCall.arguments[0] : NULL;
        ASTNode* targetNode = node->functionCall.arguments ? node->functionCall.arguments[1] : NULL;
        const char* targetText = builtinStringLiteralPayload(targetNode);
        const FisicsUnitDef* targetUnit = NULL;
        if (!targetText || !fisics_unit_lookup(targetText, &targetUnit) || !targetUnit) {
            free(args);
            *resultOut = sourceValue;
            return true;
        }
        CompilerContext* cctx = ctx->semanticModel ? semanticModelGetContext(ctx->semanticModel) : NULL;
        const FisicsUnitsExprResult* sourceResult =
            cctx ? fisics_extension_lookup_units_expr_result(cctx, sourceNode) : NULL;
        if (!sourceResult || !sourceResult->unitResolved || !sourceResult->unitDef) {
            free(args);
            *resultOut = sourceValue;
            return true;
        }
        const char* detail = NULL;
        if (!fisics_unit_can_convert(sourceResult->unitDef, targetUnit, &detail)) {
            (void)detail;
            free(args);
            *resultOut = sourceValue;
            return true;
        }
        if (sourceResult->unitDef == targetUnit) {
            free(args);
            *resultOut = sourceValue;
            return true;
        }
        LLVMTypeRef valueType = LLVMTypeOf(sourceValue);
        if (!cg_is_float_type(valueType)) {
            free(args);
            *resultOut = sourceValue;
            return true;
        }

        LLVMValueRef value = sourceValue;
        LLVMValueRef sourceScale = LLVMConstReal(valueType, sourceResult->unitDef->scale_to_canonical);
        LLVMValueRef sourceOffset = LLVMConstReal(valueType, sourceResult->unitDef->offset_to_canonical);
        LLVMValueRef targetScale = LLVMConstReal(valueType, targetUnit->scale_to_canonical);
        LLVMValueRef targetOffset = LLVMConstReal(valueType, targetUnit->offset_to_canonical);

        LLVMValueRef canonical = LLVMBuildFMul(ctx->builder, value, sourceScale, "fisics.unit.to_canonical.scale");
        if (sourceResult->unitDef->offset_to_canonical != 0.0) {
            canonical = LLVMBuildFAdd(ctx->builder,
                                      canonical,
                                      sourceOffset,
                                      "fisics.unit.to_canonical.offset");
        }
        LLVMValueRef targetValue = canonical;
        if (targetUnit->offset_to_canonical != 0.0) {
            targetValue = LLVMBuildFSub(ctx->builder,
                                        targetValue,
                                        targetOffset,
                                        "fisics.unit.from_canonical.offset");
        }
        if (targetUnit->scale_to_canonical != 1.0) {
            targetValue = LLVMBuildFDiv(ctx->builder,
                                        targetValue,
                                        targetScale,
                                        "fisics.unit.from_canonical.scale");
        }
        free(args);
        *resultOut = targetValue;
        return true;
    }

    if (isC11AtomicLoadBuiltin) {
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
        const ParsedType* ptrParsed = cg_resolve_expression_type(ctx, node->functionCall.arguments[0]);
        LLVMTypeRef valueType = cg_element_type_from_pointer(ctx, ptrParsed, LLVMTypeOf(atomicPtr));
        if (!valueType || LLVMGetTypeKind(valueType) == LLVMVoidTypeKind) {
            valueType = LLVMInt8TypeInContext(ctx->llvmContext);
        }
        LLVMTypeRef atomicType = valueType;
        if (LLVMGetTypeKind(atomicType) == LLVMIntegerTypeKind &&
            LLVMGetIntTypeWidth(atomicType) == 1) {
            atomicType = LLVMInt8TypeInContext(ctx->llvmContext);
        }
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
        LLVMValueRef typedLoad = load;
        if (atomicType != valueType) {
            if (LLVMGetTypeKind(atomicType) == LLVMIntegerTypeKind &&
                LLVMGetTypeKind(valueType) == LLVMIntegerTypeKind) {
                typedLoad = LLVMBuildIntCast2(ctx->builder,
                                              typedLoad,
                                              valueType,
                                              false,
                                              "atomic.load.bool.cast");
            } else {
                typedLoad = cg_cast_value(ctx,
                                          typedLoad,
                                          valueType,
                                          NULL,
                                          NULL,
                                          "atomic.load.result.cast");
            }
        }
        *resultOut = cg_atomic_cast_call_result(ctx, node, typedLoad, "atomic.load.result.cast");
        free(args);
        return true;
    }

    if (isC11AtomicStoreBuiltin || isC11AtomicInitBuiltin) {
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
        const ParsedType* ptrParsed = cg_resolve_expression_type(ctx, node->functionCall.arguments[0]);
        LLVMTypeRef valueType = cg_element_type_from_pointer(ctx, ptrParsed, LLVMTypeOf(atomicPtr));
        if (!valueType || LLVMGetTypeKind(valueType) == LLVMVoidTypeKind) {
            valueType = LLVMInt8TypeInContext(ctx->llvmContext);
        }
        LLVMTypeRef atomicType = valueType;
        if (LLVMGetTypeKind(atomicType) == LLVMIntegerTypeKind &&
            LLVMGetIntTypeWidth(atomicType) == 1) {
            atomicType = LLVMInt8TypeInContext(ctx->llvmContext);
        }
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
        LLVMValueRef opValue = desired;
        if (atomicType != valueType) {
            if (LLVMGetTypeKind(atomicType) == LLVMIntegerTypeKind &&
                LLVMGetTypeKind(valueType) == LLVMIntegerTypeKind) {
                opValue = LLVMBuildIntCast2(ctx->builder,
                                            opValue,
                                            atomicType,
                                            false,
                                            "atomic.store.op.cast");
            } else {
                opValue = cg_cast_value(ctx,
                                        opValue,
                                        atomicType,
                                        NULL,
                                        NULL,
                                        "atomic.store.op.cast");
            }
        }
        if (isC11AtomicStoreBuiltin) {
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

    if (isC11AtomicExchangeBuiltin) {
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
        const ParsedType* ptrParsed = cg_resolve_expression_type(ctx, node->functionCall.arguments[0]);
        LLVMTypeRef valueType = cg_element_type_from_pointer(ctx, ptrParsed, LLVMTypeOf(atomicPtr));
        if (!valueType || LLVMGetTypeKind(valueType) == LLVMVoidTypeKind) {
            valueType = LLVMInt8TypeInContext(ctx->llvmContext);
        }
        LLVMTypeRef atomicType = valueType;
        if (LLVMGetTypeKind(atomicType) == LLVMIntegerTypeKind &&
            LLVMGetIntTypeWidth(atomicType) == 1) {
            atomicType = LLVMInt8TypeInContext(ctx->llvmContext);
        }
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
        LLVMValueRef opValue = desired;
        if (atomicType != valueType) {
            if (LLVMGetTypeKind(atomicType) == LLVMIntegerTypeKind &&
                LLVMGetTypeKind(valueType) == LLVMIntegerTypeKind) {
                opValue = LLVMBuildIntCast2(ctx->builder,
                                            opValue,
                                            atomicType,
                                            false,
                                            "atomic.exchange.op.cast");
            } else {
                opValue = cg_cast_value(ctx,
                                        opValue,
                                        atomicType,
                                        NULL,
                                        NULL,
                                        "atomic.exchange.op.cast");
            }
        }
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
        LLVMValueRef typedExchange = exchange;
        if (atomicType != valueType) {
            if (LLVMGetTypeKind(atomicType) == LLVMIntegerTypeKind &&
                LLVMGetTypeKind(valueType) == LLVMIntegerTypeKind) {
                typedExchange = LLVMBuildIntCast2(ctx->builder,
                                                  typedExchange,
                                                  valueType,
                                                  false,
                                                  "atomic.exchange.bool.cast");
            } else {
                typedExchange = cg_cast_value(ctx,
                                              typedExchange,
                                              valueType,
                                              NULL,
                                              NULL,
                                              "atomic.exchange.result.cast");
            }
        }
        *resultOut = cg_atomic_cast_call_result(ctx, node, typedExchange, "atomic.exchange.result.cast");
        free(args);
        return true;
    }

    if (isAllocaBuiltin) {
        if (node->functionCall.argumentCount < 1 || !args || !args[0]) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMValueRef sizeArg = args[0];
        LLVMTypeRef sizeArgTy = LLVMTypeOf(sizeArg);
        if (!sizeArgTy) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMTypeKind sizeKind = LLVMGetTypeKind(sizeArgTy);
        if (sizeKind == LLVMPointerTypeKind) {
            sizeArg = LLVMBuildPtrToInt(ctx->builder, sizeArg, sizeType, "builtin.alloca.size.ptrtoint");
        } else if (sizeKind == LLVMIntegerTypeKind) {
            if (LLVMGetTypeKind(sizeType) == LLVMIntegerTypeKind &&
                LLVMGetIntTypeWidth(sizeArgTy) != LLVMGetIntTypeWidth(sizeType)) {
                sizeArg = LLVMBuildIntCast2(ctx->builder,
                                            sizeArg,
                                            sizeType,
                                            false,
                                            "builtin.alloca.size.cast");
            }
        } else {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMValueRef stackMem = LLVMBuildArrayAlloca(ctx->builder,
                                                     LLVMInt8TypeInContext(ctx->llvmContext),
                                                     sizeArg,
                                                     "builtin.alloca");
        const TargetLayout* tl = cg_context_get_target_layout(ctx);
        if (tl && tl->pointerAlign > 0) {
            LLVMSetAlignment(stackMem, (unsigned)tl->pointerAlign);
        }
        free(args);
        *resultOut = stackMem;
        return true;
    }

    if (isObjectSizeBuiltin) {
        LLVMTypeRef resultTy = NULL;
        const Symbol* builtinSym = cg_lookup_function_symbol_for_callee(ctx, node->functionCall.callee);
        if (builtinSym && builtinSym->kind == SYMBOL_FUNCTION) {
            resultTy = cg_type_from_parsed(ctx, &builtinSym->type);
        }
        if (!resultTy || LLVMGetTypeKind(resultTy) != LLVMIntegerTypeKind) {
            resultTy = cg_get_intptr_type(ctx);
        }
        free(args);
        *resultOut = LLVMConstAllOnes(resultTy);
        return true;
    }
    if (isInffBuiltin || isInfBuiltin || isInflBuiltin ||
        isHugeValfBuiltin || isHugeValBuiltin || isHugeVallBuiltin) {
        LLVMTypeRef infTy = (isInffBuiltin || isHugeValfBuiltin)
            ? LLVMFloatTypeInContext(ctx->llvmContext)
            : LLVMDoubleTypeInContext(ctx->llvmContext);
        free(args);
        *resultOut = LLVMConstReal(infTy, INFINITY);
        return true;
    }
    if (isNanfBuiltin || isNanBuiltin || isNanlBuiltin) {
        LLVMTypeRef nanTy = isNanfBuiltin
            ? LLVMFloatTypeInContext(ctx->llvmContext)
            : LLVMDoubleTypeInContext(ctx->llvmContext);
        free(args);
        *resultOut = LLVMConstReal(nanTy, NAN);
        return true;
    }
    if (isFabsfBuiltin || isFabsBuiltin || isFabslBuiltin) {
        if (node->functionCall.argumentCount < 1) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        const size_t keepIndices[] = {0};
        const char* targetName = isFabsfBuiltin ? "fabsf" : "fabs";
        LLVMTypeRef retTy = isFabsfBuiltin
            ? LLVMFloatTypeInContext(ctx->llvmContext)
            : LLVMDoubleTypeInContext(ctx->llvmContext);
        LLVMTypeRef argTy = retTy;
        bool promoteFlags[1] = {false};
        *resultOut = cg_emit_rewritten_builtin_call(ctx,
                                                    node,
                                                    targetName,
                                                    retTy,
                                                    args,
                                                    node->functionCall.argumentCount,
                                                    keepIndices,
                                                    1,
                                                    1,
                                                    &argTy,
                                                    promoteFlags,
                                                    false,
                                                    isFabsfBuiltin
                                                        ? "fabsf.builtin.lowered"
                                                        : (isFabslBuiltin
                                                               ? "fabsl.builtin.lowered"
                                                               : "fabs.builtin.lowered"));
        free(args);
        return true;
    }
    if (isSnprintfChkBuiltin) {
        if (node->functionCall.argumentCount < 5) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        size_t rewrittenCount = node->functionCall.argumentCount - 2;
        size_t* keepIndices = (size_t*)calloc(rewrittenCount, sizeof(size_t));
        if (!keepIndices) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        keepIndices[0] = 0;
        keepIndices[1] = 1;
        for (size_t i = 2; i < rewrittenCount; ++i) {
            keepIndices[i] = i + 2;
        }
        LLVMTypeRef snprintfParamTypes[3] = {i8PtrType, sizeType, i8PtrType};
        bool snprintfPromoteFlags[3] = {false, false, false};
        *resultOut = cg_emit_rewritten_builtin_call(ctx,
                                                    node,
                                                    "snprintf",
                                                    intType,
                                                    args,
                                                    node->functionCall.argumentCount,
                                                    keepIndices,
                                                    rewrittenCount,
                                                    3,
                                                    snprintfParamTypes,
                                                    snprintfPromoteFlags,
                                                    true,
                                                    "snprintf.chk.lowered");
        free(keepIndices);
        free(args);
        return true;
    }
    if (isVsnprintfChkBuiltin) {
        if (node->functionCall.argumentCount < 6) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        const size_t keepIndices[] = {0, 1, 4, 5};
        LLVMTypeRef vsnprintfParamTypes[4] = {
            i8PtrType, sizeType, i8PtrType, LLVMTypeOf(args[5])
        };
        bool vsnprintfPromoteFlags[4] = {false, false, false, false};
        *resultOut = cg_emit_rewritten_builtin_call(ctx,
                                                    node,
                                                    "vsnprintf",
                                                    intType,
                                                    args,
                                                    node->functionCall.argumentCount,
                                                    keepIndices,
                                                    4,
                                                    4,
                                                    vsnprintfParamTypes,
                                                    vsnprintfPromoteFlags,
                                                    false,
                                                    "vsnprintf.chk.lowered");
        free(args);
        return true;
    }
    if (isSprintfChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        size_t rewrittenCount = node->functionCall.argumentCount - 2;
        size_t* keepIndices = (size_t*)calloc(rewrittenCount, sizeof(size_t));
        if (!keepIndices) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        keepIndices[0] = 0;
        keepIndices[1] = 3;
        for (size_t i = 2; i < rewrittenCount; ++i) {
            keepIndices[i] = i + 2;
        }
        LLVMTypeRef sprintfParamTypes[2] = {i8PtrType, i8PtrType};
        bool sprintfPromoteFlags[2] = {false, false};
        *resultOut = cg_emit_rewritten_builtin_call(ctx,
                                                    node,
                                                    "sprintf",
                                                    intType,
                                                    args,
                                                    node->functionCall.argumentCount,
                                                    keepIndices,
                                                    rewrittenCount,
                                                    2,
                                                    sprintfParamTypes,
                                                    sprintfPromoteFlags,
                                                    true,
                                                    "sprintf.chk.lowered");
        free(keepIndices);
        free(args);
        return true;
    }
    if (isMemcpyChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        const size_t keepIndices[] = {0, 1, 2};
        LLVMTypeRef memcpyParamTypes[3] = {i8PtrType, i8PtrType, sizeType};
        bool memcpyPromoteFlags[3] = {false, false, false};
        *resultOut = cg_emit_rewritten_builtin_call(ctx,
                                                    node,
                                                    "memcpy",
                                                    i8PtrType,
                                                    args,
                                                    node->functionCall.argumentCount,
                                                    keepIndices,
                                                    3,
                                                    3,
                                                    memcpyParamTypes,
                                                    memcpyPromoteFlags,
                                                    false,
                                                    "memcpy.chk.lowered");
        free(args);
        return true;
    }
    if (isMemsetChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        const size_t keepIndices[] = {0, 1, 2};
        LLVMTypeRef memsetParamTypesRewrite[3] = {i8PtrType, intType, sizeType};
        bool memsetPromoteFlags[3] = {false, true, false};
        *resultOut = cg_emit_rewritten_builtin_call(ctx,
                                                    node,
                                                    "memset",
                                                    i8PtrType,
                                                    args,
                                                    node->functionCall.argumentCount,
                                                    keepIndices,
                                                    3,
                                                    3,
                                                    memsetParamTypesRewrite,
                                                    memsetPromoteFlags,
                                                    false,
                                                    "memset.chk.lowered");
        free(args);
        return true;
    }
    if (isBzeroBuiltin) {
        if (node->functionCall.argumentCount < 2) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        LLVMValueRef dst = args[0];
        LLVMValueRef sizeArg = args[1];
        if (!dst || !sizeArg) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        if (LLVMTypeOf(dst) != i8PtrType) {
            if (LLVMGetTypeKind(LLVMTypeOf(dst)) != LLVMPointerTypeKind) {
                free(args);
                *resultOut = NULL;
                return true;
            }
            dst = LLVMBuildBitCast(ctx->builder, dst, i8PtrType, "bzero.dst.cast");
        }
        if (LLVMTypeOf(sizeArg) != sizeType) {
            LLVMTypeKind sizeKind = LLVMGetTypeKind(LLVMTypeOf(sizeArg));
            if (sizeKind == LLVMIntegerTypeKind) {
                unsigned fromBits = LLVMGetIntTypeWidth(LLVMTypeOf(sizeArg));
                unsigned toBits = LLVMGetIntTypeWidth(sizeType);
                if (fromBits != toBits) {
                    sizeArg = LLVMBuildIntCast2(ctx->builder, sizeArg, sizeType, false, "bzero.size.cast");
                }
            } else if (sizeKind == LLVMPointerTypeKind) {
                sizeArg = LLVMBuildPtrToInt(ctx->builder, sizeArg, sizeType, "bzero.size.ptrtoint");
            } else {
                free(args);
                *resultOut = NULL;
                return true;
            }
        }
        LLVMTypeRef memsetParamTypes[3] = { i8PtrType, intType, sizeType };
        LLVMTypeRef memsetFnTy = LLVMFunctionType(i8PtrType, memsetParamTypes, 3, 0);
        LLVMValueRef memsetFn = LLVMGetNamedFunction(ctx->module, "memset");
        if (!memsetFn) {
            memsetFn = LLVMAddFunction(ctx->module, "memset", memsetFnTy);
        }
        LLVMValueRef memsetCallee = memsetFn;
        LLVMTypeRef memsetPtrTy = LLVMPointerType(memsetFnTy, 0);
        if (LLVMTypeOf(memsetCallee) != memsetPtrTy) {
            memsetCallee = LLVMBuildBitCast(ctx->builder, memsetCallee, memsetPtrTy, "bzero.memset.cast");
        }
        LLVMValueRef memsetArgs[3] = {
            dst,
            LLVMConstInt(intType, 0, false),
            sizeArg
        };
        (void)LLVMBuildCall2(ctx->builder,
                             memsetFnTy,
                             memsetCallee,
                             memsetArgs,
                             3,
                             "bzero.builtin.lowered");
        free(args);
        *resultOut = NULL;
        return true;
    }
    if (isStrncpyChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        const size_t keepIndices[] = {0, 1, 2};
        LLVMTypeRef strncpyParamTypes[3] = {i8PtrType, i8PtrType, sizeType};
        bool strncpyPromoteFlags[3] = {false, false, false};
        *resultOut = cg_emit_rewritten_builtin_call(ctx,
                                                    node,
                                                    "strncpy",
                                                    i8PtrType,
                                                    args,
                                                    node->functionCall.argumentCount,
                                                    keepIndices,
                                                    3,
                                                    3,
                                                    strncpyParamTypes,
                                                    strncpyPromoteFlags,
                                                    false,
                                                    "strncpy.chk.lowered");
        free(args);
        return true;
    }
    if (isStrncatChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        const size_t keepIndices[] = {0, 1, 2};
        LLVMTypeRef strncatParamTypes[3] = {i8PtrType, i8PtrType, sizeType};
        bool strncatPromoteFlags[3] = {false, false, false};
        *resultOut = cg_emit_rewritten_builtin_call(ctx,
                                                    node,
                                                    "strncat",
                                                    i8PtrType,
                                                    args,
                                                    node->functionCall.argumentCount,
                                                    keepIndices,
                                                    3,
                                                    3,
                                                    strncatParamTypes,
                                                    strncatPromoteFlags,
                                                    false,
                                                    "strncat.chk.lowered");
        free(args);
        return true;
    }
    if (isStrcpyChkBuiltin) {
        if (node->functionCall.argumentCount < 3) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        const size_t keepIndices[] = {0, 1};
        LLVMTypeRef strcpyParamTypes[2] = {i8PtrType, i8PtrType};
        bool strcpyPromoteFlags[2] = {false, false};
        *resultOut = cg_emit_rewritten_builtin_call(ctx,
                                                    node,
                                                    "strcpy",
                                                    i8PtrType,
                                                    args,
                                                    node->functionCall.argumentCount,
                                                    keepIndices,
                                                    2,
                                                    2,
                                                    strcpyParamTypes,
                                                    strcpyPromoteFlags,
                                                    false,
                                                    "strcpy.chk.lowered");
        free(args);
        return true;
    }
    if (isStrcatChkBuiltin) {
        if (node->functionCall.argumentCount < 3) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        const size_t keepIndices[] = {0, 1};
        LLVMTypeRef strcatParamTypes[2] = {i8PtrType, i8PtrType};
        bool strcatPromoteFlags[2] = {false, false};
        *resultOut = cg_emit_rewritten_builtin_call(ctx,
                                                    node,
                                                    "strcat",
                                                    i8PtrType,
                                                    args,
                                                    node->functionCall.argumentCount,
                                                    keepIndices,
                                                    2,
                                                    2,
                                                    strcatParamTypes,
                                                    strcatPromoteFlags,
                                                    false,
                                                    "strcat.chk.lowered");
        free(args);
        return true;
    }
    if (isMemmoveChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            *resultOut = NULL;
            return true;
        }
        const size_t keepIndices[] = {0, 1, 2};
        LLVMTypeRef memmoveParamTypes[3] = {i8PtrType, i8PtrType, sizeType};
        bool memmovePromoteFlags[3] = {false, false, false};
        *resultOut = cg_emit_rewritten_builtin_call(ctx,
                                                    node,
                                                    "memmove",
                                                    i8PtrType,
                                                    args,
                                                    node->functionCall.argumentCount,
                                                    keepIndices,
                                                    3,
                                                    3,
                                                    memmoveParamTypes,
                                                    memmovePromoteFlags,
                                                    false,
                                                    "memmove.chk.lowered");
        free(args);
        return true;
    }
    if (isVaStartBuiltin) {
        LLVMValueRef result = NULL;
        if (node->functionCall.argumentCount >= 1) {
            LLVMValueRef listPtr = NULL;
            LLVMTypeRef listTy = NULL;
            CGLValueInfo linfo;
            if (codegenLValue(ctx, node->functionCall.arguments[0], &listPtr, &listTy, NULL, &linfo)) {
                result = cg_emit_va_intrinsic(ctx, "llvm.va_start", listPtr);
            } else if (args) {
                result = cg_emit_va_intrinsic(ctx, "llvm.va_start", args[0]);
            }
        }
        free(args);
        *resultOut = result ? result : LLVMConstNull(LLVMInt32TypeInContext(ctx->llvmContext));
        return true;
    }
    if (isVaArgBuiltin) {
        LLVMValueRef val = NULL;
        ASTNode* vaArgTypeNode = NULL;
        if (node->functionCall.argumentCount >= 2) {
            vaArgTypeNode = node->functionCall.arguments[1];
        }
        if (vaArgTypeNode && vaArgTypeNode->type == AST_PARSED_TYPE) {
            LLVMTypeRef resTy = cg_type_from_parsed(ctx, &vaArgTypeNode->parsedTypeNode.parsed);
            if (resTy) {
                LLVMValueRef listPtr = NULL;
                LLVMTypeRef listTy = NULL;
                CGLValueInfo linfo;
                if (codegenLValue(ctx, node->functionCall.arguments[0], &listPtr, &listTy, NULL, &linfo)) {
                    val = LLVMBuildVAArg(ctx->builder, listPtr, resTy, "vaarg");
                } else if (args && node->functionCall.argumentCount >= 1 && args[0]) {
                    val = LLVMBuildVAArg(ctx->builder, args[0], resTy, "vaarg");
                }
            }
        }
        free(args);
        *resultOut = val ? val : LLVMConstNull(LLVMInt32TypeInContext(ctx->llvmContext));
        return true;
    }
    if (isVaEndBuiltin) {
        LLVMValueRef result = NULL;
        if (node->functionCall.argumentCount >= 1) {
            LLVMValueRef listPtr = NULL;
            LLVMTypeRef listTy = NULL;
            CGLValueInfo linfo;
            if (codegenLValue(ctx, node->functionCall.arguments[0], &listPtr, &listTy, NULL, &linfo)) {
                result = cg_emit_va_intrinsic(ctx, "llvm.va_end", listPtr);
            } else if (args) {
                result = cg_emit_va_intrinsic(ctx, "llvm.va_end", args[0]);
            }
        }
        free(args);
        *resultOut = result ? result : LLVMConstNull(LLVMInt32TypeInContext(ctx->llvmContext));
        return true;
    }
    if (isVaCopyBuiltin) {
        LLVMValueRef result = NULL;
        if (node->functionCall.argumentCount >= 2) {
            LLVMValueRef dstPtr = NULL;
            LLVMTypeRef dstTy = NULL;
            CGLValueInfo dstInfo;
            LLVMValueRef srcPtr = NULL;
            LLVMTypeRef srcTy = NULL;
            CGLValueInfo srcInfo;
            LLVMValueRef srcVal = NULL;

            bool haveDst = codegenLValue(ctx, node->functionCall.arguments[0], &dstPtr, &dstTy, NULL, &dstInfo);
            bool haveSrc = codegenLValue(ctx, node->functionCall.arguments[1], &srcPtr, &srcTy, NULL, &srcInfo);

            if (haveSrc && srcPtr) {
                LLVMTypeRef loadTy = srcTy;
                if (!loadTy) {
                    loadTy = cg_dereference_ptr_type(ctx, LLVMTypeOf(srcPtr), "va_copy src");
                }
                if (loadTy && LLVMGetTypeKind(loadTy) != LLVMVoidTypeKind) {
                    srcVal = LLVMBuildLoad2(ctx->builder, loadTy, srcPtr, "va.copy.load");
                }
            } else if (args) {
                srcVal = args[1];
            }

            if (haveDst && dstPtr && srcVal) {
                LLVMTypeRef storeTy = dstTy;
                if (!storeTy) {
                    storeTy = cg_dereference_ptr_type(ctx, LLVMTypeOf(dstPtr), "va_copy dst");
                }
                if (!storeTy || LLVMGetTypeKind(storeTy) == LLVMVoidTypeKind) {
                    storeTy = LLVMTypeOf(srcVal);
                }
                if (storeTy && LLVMTypeOf(srcVal) != storeTy) {
                    LLVMTypeKind srcKind = LLVMGetTypeKind(LLVMTypeOf(srcVal));
                    LLVMTypeKind dstKind = LLVMGetTypeKind(storeTy);
                    if (srcKind == LLVMPointerTypeKind && dstKind == LLVMPointerTypeKind) {
                        srcVal = LLVMBuildBitCast(ctx->builder, srcVal, storeTy, "va.copy.cast");
                    } else if (srcKind == LLVMIntegerTypeKind && dstKind == LLVMIntegerTypeKind) {
                        bool srcSigned = !cg_expression_is_unsigned(ctx, node->functionCall.arguments[1]);
                        srcVal = LLVMBuildIntCast2(ctx->builder, srcVal, storeTy, srcSigned, "va.copy.cast");
                    }
                }
                result = LLVMBuildStore(ctx->builder, srcVal, dstPtr);
            }
        }
        free(args);
        *resultOut = result ? result : LLVMConstNull(LLVMInt32TypeInContext(ctx->llvmContext));
        return true;
    }
    if (isExpectBuiltin) {
        if (node->functionCall.argumentCount >= 2 && args && args[0] && args[1]) {
            LLVMValueRef val = args[0];
            LLVMValueRef expectVal = args[1];
            LLVMTypeRef valTy = LLVMTypeOf(val);
            if (!valTy || LLVMGetTypeKind(valTy) != LLVMIntegerTypeKind) {
                valTy = cg_get_intptr_type(ctx);
                val = ensureIntegerLike(ctx, val);
            }
            LLVMTypeRef expectTy = LLVMTypeOf(expectVal);
            if (!expectTy || LLVMGetTypeKind(expectTy) != LLVMIntegerTypeKind ||
                LLVMGetIntTypeWidth(expectTy) != LLVMGetIntTypeWidth(valTy)) {
                expectVal = LLVMBuildIntCast2(ctx->builder, expectVal, valTy, 0, "expect.cast");
            }
            if (!LLVMIsAConstantInt(expectVal)) {
                free(args);
                *resultOut = val;
                return true;
            }

            unsigned bits = LLVMGetIntTypeWidth(valTy);
            if (bits == 0) bits = 64;
            char name[32];
            snprintf(name, sizeof(name), "llvm.expect.i%u", bits);
            LLVMTypeRef expectParamTypes[2] = {valTy, valTy};
            LLVMTypeRef fnTy = LLVMFunctionType(valTy, expectParamTypes, 2, 0);
            LLVMValueRef fn = LLVMGetNamedFunction(ctx->module, name);
            if (!fn) {
                fn = LLVMAddFunction(ctx->module, name, fnTy);
            }
            LLVMValueRef expectArgs[2] = {val, expectVal};
            *resultOut = LLVMBuildCall2(ctx->builder, fnTy, fn, expectArgs, 2, "expect");
            free(args);
            return true;
        }
        free(args);
        *resultOut = LLVMConstNull(LLVMInt32TypeInContext(ctx->llvmContext));
        return true;
    }

    return false;
}
