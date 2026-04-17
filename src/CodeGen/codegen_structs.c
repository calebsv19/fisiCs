// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"
#include "Compiler/compiler_context.h"
#include "Syntax/layout.h"

#include "codegen_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static LLVMValueRef cg_structs_bitfield_mask(LLVMTypeRef storageTy, unsigned width) {
    unsigned storageBits = LLVMGetIntTypeWidth(storageTy);
    if (width == 0) {
        return LLVMConstInt(storageTy, 0, 0);
    }
    if (width >= storageBits) {
        return LLVMConstAllOnes(storageTy);
    }
    uint64_t mask = (width >= 64) ? ~0ULL : ((1ULL << width) - 1ULL);
    return LLVMConstInt(storageTy, mask, 0);
}

static LLVMValueRef cg_structs_load_bitfield(CodegenContext* ctx,
                                             const CGLValueInfo* info,
                                             LLVMTypeRef resultType) {
    if (!ctx || !info || !info->isBitfield) return NULL;
    LLVMValueRef raw = LLVMBuildLoad2(ctx->builder, info->storageType, info->storagePtr, "bf.load");
    unsigned bitOffset = (unsigned)info->layout.bitOffset;
    LLVMTypeRef storageTy = info->storageType;
    LLVMValueRef shifted = raw;
    if (bitOffset > 0) {
        LLVMValueRef sh = LLVMConstInt(storageTy, bitOffset, 0);
        shifted = LLVMBuildLShr(ctx->builder, raw, sh, "bf.shr");
    }
    LLVMValueRef mask = cg_structs_bitfield_mask(storageTy, (unsigned)info->layout.widthBits);
    LLVMValueRef masked = LLVMBuildAnd(ctx->builder, shifted, mask, "bf.mask");
    if (!resultType) {
        unsigned width = info->layout.widthBits ? (unsigned)info->layout.widthBits : LLVMGetIntTypeWidth(storageTy);
        if (width == 0) width = LLVMGetIntTypeWidth(storageTy);
        resultType = LLVMIntTypeInContext(ctx->llvmContext, width);
    }
    if (LLVMGetTypeKind(resultType) != LLVMIntegerTypeKind) {
        return LLVMBuildBitCast(ctx->builder, masked, resultType, "bf.cast");
    }
    unsigned destBits = LLVMGetIntTypeWidth(resultType);
    if (destBits > LLVMGetIntTypeWidth(storageTy)) {
        if (info->layout.isSigned) {
            return LLVMBuildSExt(ctx->builder, masked, resultType, "bf.sext");
        }
        return LLVMBuildZExt(ctx->builder, masked, resultType, "bf.zext");
    }
    if (destBits < LLVMGetIntTypeWidth(storageTy)) {
        return LLVMBuildTrunc(ctx->builder, masked, resultType, "bf.trunc");
    }
    return masked;
}

static bool cg_structs_has_pointer_layer(const ParsedType* type) {
    if (!type) return false;
    if (type->isFunctionPointer) return true;
    if (type->pointerDepth > 0) return true;
    for (size_t i = 0; i < type->derivationCount; ++i) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(type, i);
        if (deriv && deriv->kind == TYPE_DERIVATION_POINTER) {
            return true;
        }
    }
    return false;
}

static void recordStructInfo(CodegenContext* ctx,
                             const char* name,
                             bool isUnion,
                             StructFieldInfo* fields,
                             size_t fieldCount,
                             LLVMTypeRef llvmType) {
    if (!ctx || !name) {
        if (fields) free(fields);
        return;
    }

    for (size_t i = 0; i < ctx->structInfoCount; ++i) {
        if (ctx->structInfos[i].name && strcmp(ctx->structInfos[i].name, name) == 0) {
            for (size_t j = 0; j < ctx->structInfos[i].fieldCount; ++j) {
                free(ctx->structInfos[i].fields[j].name);
            }
            free(ctx->structInfos[i].fields);
            ctx->structInfos[i].fields = fields;
            ctx->structInfos[i].fieldCount = fieldCount;
            ctx->structInfos[i].isUnion = isUnion;
            ctx->structInfos[i].llvmType = llvmType;
            return;
        }
    }

    if (ctx->structInfoCount == ctx->structInfoCapacity) {
        size_t newCap = ctx->structInfoCapacity ? ctx->structInfoCapacity * 2 : 4;
        StructInfo* resized = (StructInfo*)realloc(ctx->structInfos, newCap * sizeof(StructInfo));
        if (!resized) {
            for (size_t j = 0; j < fieldCount; ++j) {
                free(fields[j].name);
            }
            free(fields);
            return;
        }
        ctx->structInfos = resized;
        ctx->structInfoCapacity = newCap;
    }

    ctx->structInfos[ctx->structInfoCount].name = strdup(name);
    ctx->structInfos[ctx->structInfoCount].fields = fields;
    ctx->structInfos[ctx->structInfoCount].fieldCount = fieldCount;
    ctx->structInfos[ctx->structInfoCount].isUnion = isUnion;
    ctx->structInfos[ctx->structInfoCount].llvmType = llvmType;
    ctx->structInfoCount += 1;
}

LLVMTypeRef ensureStructLLVMTypeByName(CodegenContext* ctx, const char* name, bool isUnionHint) {
    if (!ctx || !name || name[0] == '\0') return NULL;
    ParsedType temp;
    memset(&temp, 0, sizeof(temp));
    temp.kind = isUnionHint ? TYPE_UNION : TYPE_STRUCT;
    temp.userTypeName = (char*)name;
    return cg_type_from_parsed(ctx, &temp);
}

void declareStructSymbol(CodegenContext* ctx, const Symbol* sym) {
    if (!ctx || !sym) return;
    bool isUnion = false;
    if (sym->definition &&
        (sym->definition->type == AST_STRUCT_DEFINITION || sym->definition->type == AST_UNION_DEFINITION)) {
        isUnion = (sym->definition->type == AST_UNION_DEFINITION);
        codegenStructDefinition(ctx, sym->definition);
    } else if (sym->name) {
        CGTypeCache* cache = cg_context_get_type_cache(ctx);
        if (cache) {
            cg_type_cache_lookup_struct(cache, sym->name, &isUnion);
        }
        ensureStructLLVMTypeByName(ctx, sym->name, isUnion);
    }
}

bool codegenLValue(CodegenContext* ctx,
                   ASTNode* target,
                   LLVMValueRef* outPtr,
                   LLVMTypeRef* outType,
                   const ParsedType** outParsedType,
                   CGLValueInfo* outInfo) {
    if (!ctx || !target || !outPtr || !outType) return false;
    if (outParsedType) {
        *outParsedType = NULL;
    }
    if (outInfo) {
        memset(outInfo, 0, sizeof(*outInfo));
    }

    switch (target->type) {
        case AST_IDENTIFIER: {
            const char* name = target->valueNode.value;
            const Symbol* sym = NULL;
            const SemanticModel* model = cg_context_get_semantic_model(ctx);
            NamedValue* entry = cg_scope_lookup(ctx->currentScope, name);
            if (!entry) {
                if (model) {
                    sym = semanticModelLookupGlobal(model, name);
                    if (sym) {
                        if (sym->kind == SYMBOL_VARIABLE) {
                            declareGlobalVariableSymbol(ctx, sym);
                        } else if (sym->kind == SYMBOL_FUNCTION) {
                            declareFunctionSymbol(ctx, sym);
                        }
                        entry = cg_scope_lookup(ctx->currentScope, name);
                    }
                }
            }
            if (!entry) {
                /*
                 * Function designators are not lvalues. Callers such as unary '&'
                 * can legitimately fall back to function-pointer handling, so do
                 * not emit a hard codegen error for that path.
                 */
                if (sym && sym->kind == SYMBOL_FUNCTION) {
                    return false;
                }
                if (name && LLVMGetNamedFunction(ctx->module, name)) {
                    return false;
                }
                fprintf(stderr, "Error: assignment target '%s' not declared\n", name ? name : "<anonymous>");
                return false;
            }
            *outPtr = entry->value;
            if (entry->addressOnly) {
                *outType = entry->type;
                if (outParsedType) {
                    *outParsedType = entry->parsedType;
                }
                return true;
            }
            *outType = entry->type ? entry->type : cg_dereference_ptr_type(ctx, LLVMTypeOf(entry->value), "cached lvalue");
            if (outParsedType) {
                *outParsedType = entry->parsedType;
            }
            return true;
        }
        case AST_ARRAY_ACCESS: {
            LLVMValueRef arrayPtr = NULL;
            LLVMTypeRef baseType = NULL;
            const ParsedType* baseParsed = NULL;
            CGLValueInfo baseInfo;
            bool haveBasePtr =
                codegenLValue(ctx, target->arrayAccess.array, &arrayPtr, &baseType, &baseParsed, &baseInfo);
            if (!haveBasePtr) {
                arrayPtr = codegenNode(ctx, target->arrayAccess.array);
                baseParsed = cg_resolve_expression_type(ctx, target->arrayAccess.array);
            }
            if (!baseParsed && target->arrayAccess.array) {
                baseParsed = cg_resolve_expression_type(ctx, target->arrayAccess.array);
            }
            const ParsedType* refinedCallParsed =
                cg_refine_function_call_result_type(ctx, target->arrayAccess.array);
            if (refinedCallParsed) {
                baseParsed = refinedCallParsed;
            }
            if (!arrayPtr) return false;
            /* If the base is a pointer variable (not an actual array object), load the pointer
               value so we index the pointee rather than the stack slot. */
            bool baseIsDirectArray = (baseParsed && parsedTypeIsDirectArray(baseParsed)) ||
                                     (baseType && LLVMGetTypeKind(baseType) == LLVMArrayTypeKind);
            bool baseNeedsLoad = haveBasePtr && !baseIsDirectArray;
            if (baseNeedsLoad) {
                LLVMTypeRef loadTy = baseType;
                if (!loadTy) {
                    LLVMTypeRef elem = NULL;
                    if (arrayPtr && LLVMGetTypeKind(LLVMTypeOf(arrayPtr)) == LLVMPointerTypeKind) {
                        LLVMTypeRef arrayPtrTy = LLVMTypeOf(arrayPtr);
                        if (!LLVMPointerTypeIsOpaque(arrayPtrTy)) {
                            elem = LLVMGetElementType(arrayPtrTy);
                        }
                    }
                    loadTy = elem ? elem : LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
                }
                arrayPtr = LLVMBuildLoad2(ctx->builder, loadTy, arrayPtr, "array.ptr.load");
                baseType = loadTy;
            }
            LLVMValueRef index = codegenNode(ctx, target->arrayAccess.index);
            if (baseParsed && parsedTypeIsDirectArray(baseParsed) && parsedTypeHasVLA(baseParsed)) {
                LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
                LLVMValueRef offset = index;
                LLVMTypeRef idxTy = LLVMTypeOf(offset);
                if (!idxTy || LLVMGetTypeKind(idxTy) != LLVMIntegerTypeKind) {
                    return false;
                }
                if (idxTy != intptrTy) {
                    unsigned idxBits = LLVMGetIntTypeWidth(idxTy);
                    unsigned ptrBits = LLVMGetIntTypeWidth(intptrTy);
                    if (idxBits != ptrBits) {
                        offset = LLVMBuildIntCast2(ctx->builder, offset, intptrTy, 0, "vla.idx.cast");
                    }
                }

                size_t arrayDims = 0;
                for (size_t d = 0; d < baseParsed->derivationCount; ++d) {
                    const TypeDerivation* deriv = parsedTypeGetDerivation(baseParsed, d);
                    if (deriv && deriv->kind == TYPE_DERIVATION_ARRAY) {
                        arrayDims++;
                    }
                }
                if (arrayDims > 1) {
                    LLVMValueRef stride = LLVMConstInt(intptrTy, 1, 0);
                    for (size_t d = 1; d < baseParsed->derivationCount; ++d) {
                        const TypeDerivation* deriv = parsedTypeGetDerivation(baseParsed, d);
                        if (!deriv || deriv->kind != TYPE_DERIVATION_ARRAY) {
                            continue;
                        }
                        LLVMValueRef dim = NULL;
                        if (!deriv->as.array.isVLA &&
                            deriv->as.array.hasConstantSize &&
                            deriv->as.array.constantSize > 0) {
                            dim = LLVMConstInt(intptrTy, (unsigned long long)deriv->as.array.constantSize, 0);
                        } else if (deriv->as.array.sizeExpr) {
                            LLVMValueRef evaluated = codegenNode(ctx, deriv->as.array.sizeExpr);
                            if (evaluated && LLVMGetTypeKind(LLVMTypeOf(evaluated)) == LLVMIntegerTypeKind) {
                                if (LLVMTypeOf(evaluated) != intptrTy) {
                                    unsigned evalBits = LLVMGetIntTypeWidth(LLVMTypeOf(evaluated));
                                    unsigned ptrBits = LLVMGetIntTypeWidth(intptrTy);
                                    if (evalBits != ptrBits) {
                                        evaluated = LLVMBuildIntCast2(ctx->builder, evaluated, intptrTy, 0, "vla.dim.cast");
                                    }
                                }
                                dim = evaluated;
                            }
                        }
                        if (!dim) {
                            dim = LLVMConstInt(intptrTy, 1, 0);
                        }
                        stride = LLVMBuildMul(ctx->builder, stride, dim, "vla.stride");
                    }
                    offset = LLVMBuildMul(ctx->builder, offset, stride, "vla.offset");
                }

                ParsedType remainingParsed = parsedTypeArrayElementType(baseParsed);
                ParsedType scalarParsed = parsedTypeClone(baseParsed);
                while (parsedTypeIsDirectArray(&scalarParsed)) {
                    ParsedType next = parsedTypeArrayElementType(&scalarParsed);
                    parsedTypeFree(&scalarParsed);
                    scalarParsed = next;
                }
                LLVMTypeRef scalarType = cg_type_from_parsed(ctx, &scalarParsed);
                if (!scalarType || LLVMGetTypeKind(scalarType) == LLVMVoidTypeKind) {
                    scalarType = LLVMInt32TypeInContext(ctx->llvmContext);
                }
                LLVMTypeRef resultType = NULL;
                if (remainingParsed.kind != TYPE_INVALID) {
                    resultType = cg_type_from_parsed(ctx, &remainingParsed);
                }
                if (!resultType || LLVMGetTypeKind(resultType) == LLVMVoidTypeKind) {
                    resultType = scalarType;
                }

                LLVMValueRef elementPtr =
                    LLVMBuildGEP2(ctx->builder, scalarType, arrayPtr, &offset, 1, "vla.elem.ptr");
                if (!elementPtr) {
                    parsedTypeFree(&remainingParsed);
                    parsedTypeFree(&scalarParsed);
                    return false;
                }
                if (outInfo) {
                    outInfo->isFlexElement = baseInfo.isFlexBase || baseInfo.isFlexElement;
                    outInfo->flexElemSize = baseInfo.flexElemSize;
                }
                *outPtr = elementPtr;
                *outType = resultType;
                if (outParsedType) {
                    static ParsedType tmp;
                    parsedTypeFree(&tmp);
                    if (remainingParsed.kind != TYPE_INVALID) {
                        tmp = parsedTypeClone(&remainingParsed);
                    } else {
                        tmp.kind = TYPE_INVALID;
                    }
                    *outParsedType = (tmp.kind != TYPE_INVALID) ? &tmp : NULL;
                }
                parsedTypeFree(&remainingParsed);
                parsedTypeFree(&scalarParsed);
                return true;
            }
            LLVMTypeRef aggregateHint = NULL;
            if (baseParsed && parsedTypeIsDirectArray(baseParsed)) {
                aggregateHint = cg_type_from_parsed(ctx, baseParsed);
            } else if (baseType && LLVMGetTypeKind(baseType) == LLVMArrayTypeKind) {
                aggregateHint = baseType;
            }
            LLVMTypeRef elementHint = NULL;
            /* Prefer concrete LLVM element hints from the base value type first. */
            if (baseType && LLVMGetTypeKind(baseType) == LLVMArrayTypeKind) {
                elementHint = LLVMGetElementType(baseType);
            } else if (baseType &&
                       LLVMGetTypeKind(baseType) == LLVMPointerTypeKind &&
                       !LLVMPointerTypeIsOpaque(baseType)) {
                LLVMTypeRef elem = LLVMGetElementType(baseType);
                if (elem) {
                    elementHint = elem;
                }
            }
            if (!elementHint) {
                elementHint = cg_element_type_hint_from_parsed(ctx, baseParsed);
            }
            LLVMTypeRef elementType = NULL;
            LLVMValueRef elementPtr = buildArrayElementPointer(ctx,
                                                               arrayPtr,
                                                               index,
                                                               baseParsed,
                                                               aggregateHint,
                                                               elementHint,
                                                               &elementType);
            if (!elementPtr) return false;
            if (!elementType) {
                elementType = cg_dereference_ptr_type(ctx, LLVMTypeOf(elementPtr), "array element load");
            }
            if (elementType && LLVMGetTypeKind(elementType) == LLVMFunctionTypeKind) {
                elementType = LLVMPointerType(elementType, 0);
            }
            if (outInfo) {
                outInfo->isFlexElement = baseInfo.isFlexBase || baseInfo.isFlexElement;
                outInfo->flexElemSize = baseInfo.flexElemSize;
                if (outInfo->isFlexElement && outInfo->flexElemSize == 0) {
                    uint64_t sz = 0;
                    uint32_t al = 0;
                    cg_size_align_for_type(ctx, baseParsed, elementType, &sz, &al);
                    if (sz == 0 && baseParsed && parsedTypeIsDirectArray(baseParsed)) {
                        ParsedType elem = parsedTypeArrayElementType(baseParsed);
                        cg_size_align_for_type(ctx, &elem, elementType, &sz, &al);
                        parsedTypeFree(&elem);
                    }
                    outInfo->flexElemSize = sz ? sz : 1;
                }
                if (!outInfo->isFlexElement && baseParsed && parsedTypeIsDirectArray(baseParsed)) {
                    const TypeDerivation* d = parsedTypeGetDerivation(baseParsed, baseParsed->derivationCount - 1);
                    if (d && d->kind == TYPE_DERIVATION_ARRAY && d->as.array.isFlexible) {
                        outInfo->isFlexElement = true;
                        uint64_t sz = 0;
                        uint32_t al = 0;
                        cg_size_align_for_type(ctx, baseParsed, elementType, &sz, &al);
                        outInfo->flexElemSize = sz ? sz : 1;
                    }
                }
            }
            *outPtr = elementPtr;
            *outType = elementType ? elementType : LLVMInt8TypeInContext(ctx->llvmContext);
            if (outParsedType) {
                const ParsedType* elemParsed = cg_resolve_expression_type(ctx, target->arrayAccess.array);
                if (elemParsed && parsedTypeIsDirectArray(elemParsed)) {
                    static ParsedType tmp;
                    parsedTypeFree(&tmp);
                    tmp = parsedTypeArrayElementType(elemParsed);
                    *outParsedType = &tmp;
                } else if (elemParsed && cg_structs_has_pointer_layer(elemParsed)) {
                    ParsedType pointed = parsedTypePointerTargetType(elemParsed);
                    if (pointed.kind != TYPE_INVALID && parsedTypeIsDirectArray(&pointed)) {
                        static ParsedType tmp;
                        parsedTypeFree(&tmp);
                        tmp = pointed;
                        *outParsedType = &tmp;
                    } else {
                        parsedTypeFree(&pointed);
                    }
                }
                if (!*outParsedType) {
                    /*
                     * Fallback to semantic type of the full subscript expression.
                     * This preserves pointee aggregate metadata for chains like:
                     *   ptr_to_struct_array[i].field
                     */
                    const ParsedType* resolvedElement = cg_resolve_expression_type(ctx, target);
                    if (resolvedElement) {
                        *outParsedType = resolvedElement;
                    }
                }
            }
            return true;
        }
        case AST_UNARY_EXPRESSION: {
            if (!target->expr.op || strcmp(target->expr.op, "*") != 0) {
                return false;
            }
            LLVMValueRef pointerValue = codegenNode(ctx, target->expr.left);
            if (!pointerValue) return false;
            const ParsedType* ptrParsed = cg_resolve_expression_type(ctx, target->expr.left);
            LLVMTypeRef elemType = cg_element_type_from_pointer(ctx, ptrParsed, LLVMTypeOf(pointerValue));
            if (!elemType || LLVMGetTypeKind(elemType) == LLVMVoidTypeKind) {
                elemType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            *outPtr = pointerValue;
            *outType = elemType;
            if (outParsedType && ptrParsed) {
                static ParsedType tmpSlots[8];
                static size_t tmpSlot = 0;
                ParsedType* slot = &tmpSlots[tmpSlot % 8];
                tmpSlot++;
                parsedTypeFree(slot);
                *slot = parsedTypePointerTargetType(ptrParsed);
                if (slot->kind != TYPE_INVALID) {
                    *outParsedType = slot;
                }
            }
            return true;
        }
        case AST_POINTER_DEREFERENCE: {
            LLVMValueRef pointerValue = codegenNode(ctx, target->pointerDeref.pointer);
            if (!pointerValue) return false;
            const ParsedType* ptrParsed = cg_resolve_expression_type(ctx, target->pointerDeref.pointer);
            LLVMTypeRef elemType = cg_element_type_from_pointer(ctx, ptrParsed, LLVMTypeOf(pointerValue));
            if (!elemType || LLVMGetTypeKind(elemType) == LLVMVoidTypeKind) {
                elemType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            *outPtr = pointerValue;
            *outType = elemType;
            if (outParsedType && ptrParsed) {
                static ParsedType tmpSlots[8];
                static size_t tmpSlot = 0;
                ParsedType* slot = &tmpSlots[tmpSlot % 8];
                tmpSlot++;
                parsedTypeFree(slot);
                *slot = parsedTypePointerTargetType(ptrParsed);
                if (slot->kind != TYPE_INVALID) {
                    *outParsedType = slot;
                }
            }
            return true;
        }
        case AST_DOT_ACCESS:
        case AST_STRUCT_FIELD_ACCESS: {
            ASTNode* baseNode = (target->type == AST_STRUCT_FIELD_ACCESS)
                ? target->structFieldAccess.structInstance
                : target->memberAccess.base;
            const char* fieldName = (target->type == AST_STRUCT_FIELD_ACCESS)
                ? target->structFieldAccess.fieldName
                : target->memberAccess.field;
            LLVMValueRef basePtr = NULL;
            LLVMTypeRef baseType = NULL;
            const ParsedType* baseParsed = NULL;
            if (!codegenLValue(ctx, baseNode, &basePtr, &baseType, &baseParsed, NULL)) {
                LLVMValueRef baseVal = codegenNode(ctx, baseNode);
                if (!baseVal) return false;
                if (LLVMGetTypeKind(LLVMTypeOf(baseVal)) == LLVMPointerTypeKind) {
                    basePtr = baseVal;
                    baseType = cg_dereference_ptr_type(ctx, LLVMTypeOf(baseVal), "dot access");
                } else {
                    LLVMValueRef tmpAlloca = cg_build_entry_alloca(ctx, LLVMTypeOf(baseVal), "dot_tmp_lhs");
                    LLVMBuildStore(ctx->builder, baseVal, tmpAlloca);
                    basePtr = tmpAlloca;
                    baseType = LLVMTypeOf(baseVal);
                }
                if (!baseParsed) {
                    baseParsed = cg_resolve_expression_type(ctx, baseNode);
                }
            }

            const char* nameHint = NULL;
            if (baseType && LLVMGetTypeKind(baseType) == LLVMStructTypeKind) {
                nameHint = LLVMGetStructName(baseType);
            }

            LLVMTypeRef fieldType = NULL;
            const ParsedType* fieldParsed = NULL;
            const CCTagFieldLayout* lay = cg_lookup_field_layout(ctx, baseParsed, fieldName);
            if (lay && lay->isBitfield && lay->widthBits > 0 && outInfo) {
                unsigned storageBits = (unsigned)(lay->storageUnitBytes ? lay->storageUnitBytes * 8 : 32);
                LLVMTypeRef storageTy = LLVMIntTypeInContext(ctx->llvmContext, storageBits);
                LLVMValueRef baseI8 = LLVMBuildBitCast(ctx->builder,
                                                       basePtr,
                                                       LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0),
                                                       "bf.base");
                LLVMValueRef offsetVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), lay->byteOffset, 0);
                LLVMValueRef ptrI8 = LLVMBuildGEP2(ctx->builder,
                                                   LLVMInt8TypeInContext(ctx->llvmContext),
                                                   baseI8,
                                                   &offsetVal,
                                                   1,
                                                   "bf.gep");
                LLVMValueRef storagePtr = LLVMBuildBitCast(ctx->builder,
                                                           ptrI8,
                                                           LLVMPointerType(storageTy, 0),
                                                           "bf.ptr");
                *outPtr = storagePtr;
                *outType = storageTy;
                if (outParsedType) *outParsedType = fieldParsed;
                outInfo->isBitfield = true;
                outInfo->layout = *lay;
                outInfo->storagePtr = storagePtr;
                outInfo->storageType = storageTy;
                outInfo->fieldParsed = fieldParsed;
                return true;
            }
            LLVMValueRef fieldPtr = buildStructFieldPointer(ctx,
                                                            basePtr,
                                                            baseType,
                                                            nameHint,
                                                            fieldName,
                                                            baseParsed,
                                                            &fieldType,
                                                            &fieldParsed);
            if (!fieldPtr) return false;
            *outPtr = fieldPtr;
            if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
                fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            *outType = fieldType;
            if (outParsedType) {
                *outParsedType = fieldParsed;
            }
            if (outInfo && fieldParsed && parsedTypeIsDirectArray(fieldParsed)) {
                const TypeDerivation* d = parsedTypeGetDerivation(fieldParsed, fieldParsed->derivationCount - 1);
                if (d && d->kind == TYPE_DERIVATION_ARRAY && d->as.array.isFlexible) {
                    outInfo->isFlexBase = true;
                    uint64_t size = 0;
                    uint32_t al = 0;
                    ParsedType elem = parsedTypeArrayElementType(fieldParsed);
                    cg_size_align_for_type(ctx, &elem, fieldType, &size, &al);
                    parsedTypeFree(&elem);
                    outInfo->flexElemSize = size ? size : 1;
                }
            }
            if (outInfo && !outInfo->isFlexBase && lay && !lay->isBitfield && lay->storageUnitBytes == 0) {
                outInfo->isFlexBase = true;
                uint64_t size = 0;
                uint32_t al = 0;
                if (fieldParsed && parsedTypeIsDirectArray(fieldParsed)) {
                    ParsedType elem = parsedTypeArrayElementType(fieldParsed);
                    cg_size_align_for_type(ctx, &elem, fieldType, &size, &al);
                    parsedTypeFree(&elem);
                }
                if (size == 0) {
                    cg_size_align_for_type(ctx, NULL, fieldType, &size, &al);
                }
                outInfo->flexElemSize = size ? size : 1;
            }
            return true;
        }
        case AST_POINTER_ACCESS: {
            const char* dbgPtr = getenv("FISICS_DEBUG_PTR_ACCESS");
            if (dbgPtr && dbgPtr[0] && strcmp(dbgPtr, "0") != 0) {
                fprintf(stderr,
                        "[ptr-access] line=%d field=%s\n",
                        target->line,
                        target->memberAccess.field ? target->memberAccess.field : "<null>");
            }
            LLVMValueRef baseValue = codegenNode(ctx, target->memberAccess.base);
            if (!baseValue) return false;
            if (LLVMGetTypeKind(LLVMTypeOf(baseValue)) != LLVMPointerTypeKind) {
                fprintf(stderr, "Error: Cannot apply '->' to non-pointer type\n");
                return false;
            }
            LLVMValueRef basePtr = baseValue;
            LLVMTypeRef baseType = cg_dereference_ptr_type(ctx, LLVMTypeOf(baseValue), "pointer member access");
            const ParsedType* baseParsed = cg_resolve_expression_type(ctx, target->memberAccess.base);
            ParsedType pointed = parsedTypePointerTargetType(baseParsed);
            ParsedType arrayElem;
            memset(&arrayElem, 0, sizeof(arrayElem));
            arrayElem.kind = TYPE_INVALID;
            bool haveArrayElem = false;
            bool havePointed = (pointed.kind != TYPE_INVALID);
            const ParsedType* structHint = NULL;
            if (havePointed) {
                structHint = &pointed;
            } else if (baseParsed && parsedTypeIsDirectArray(baseParsed)) {
                arrayElem = parsedTypeArrayElementType(baseParsed);
                if (arrayElem.kind != TYPE_INVALID) {
                    haveArrayElem = true;
                    structHint = &arrayElem;
                }
            }
            if (!structHint) {
                structHint = baseParsed;
            }
            const char* nameHint = NULL;
            if (baseType && LLVMGetTypeKind(baseType) == LLVMStructTypeKind) {
                nameHint = LLVMGetStructName(baseType);
            }
            const CCTagFieldLayout* lay = cg_lookup_field_layout(ctx, structHint, target->memberAccess.field);
            if (lay && lay->isBitfield && lay->widthBits > 0 && outInfo) {
                unsigned storageBits = (unsigned)(lay->storageUnitBytes ? lay->storageUnitBytes * 8 : 32);
                LLVMTypeRef storageTy = LLVMIntTypeInContext(ctx->llvmContext, storageBits);
                LLVMValueRef baseI8 = LLVMBuildBitCast(ctx->builder,
                                                       basePtr,
                                                       LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0),
                                                       "bf.base");
                LLVMValueRef offsetVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), lay->byteOffset, 0);
                LLVMValueRef ptrI8 = LLVMBuildGEP2(ctx->builder,
                                                   LLVMInt8TypeInContext(ctx->llvmContext),
                                                   baseI8,
                                                   &offsetVal,
                                                   1,
                                                   "bf.gep");
                LLVMValueRef storagePtr = LLVMBuildBitCast(ctx->builder,
                                                           ptrI8,
                                                           LLVMPointerType(storageTy, 0),
                                                           "bf.ptr");
                *outPtr = storagePtr;
                *outType = storageTy;
                outInfo->isBitfield = true;
                outInfo->layout = *lay;
                outInfo->storagePtr = storagePtr;
                outInfo->storageType = storageTy;
                outInfo->fieldParsed = NULL;
                if (havePointed) {
                    parsedTypeFree(&pointed);
                }
                if (haveArrayElem) {
                    parsedTypeFree(&arrayElem);
                }
                return true;
            }
            LLVMTypeRef fieldType = NULL;
            const ParsedType* fieldParsed = NULL;
            LLVMValueRef fieldPtr = buildStructFieldPointer(ctx,
                                                            basePtr,
                                                            baseType,
                                                            nameHint,
                                                            target->memberAccess.field,
                                                            structHint,
                                                            &fieldType,
                                                            &fieldParsed);
            if (!fieldPtr) {
                if (dbgPtr && dbgPtr[0] && strcmp(dbgPtr, "0") != 0) {
                    fprintf(stderr,
                            "[ptr-access] lookup-fail line=%d field=%s\n",
                            target->line,
                            target->memberAccess.field ? target->memberAccess.field : "<null>");
                }
                if (havePointed) parsedTypeFree(&pointed);
                if (haveArrayElem) parsedTypeFree(&arrayElem);
                return false;
            }
            *outPtr = fieldPtr;
            *outType = fieldType ? fieldType : LLVMInt32TypeInContext(ctx->llvmContext);
            if (outParsedType) {
                *outParsedType = fieldParsed;
            }
            if (outInfo && fieldParsed && parsedTypeIsDirectArray(fieldParsed)) {
                const TypeDerivation* d = parsedTypeGetDerivation(fieldParsed, fieldParsed->derivationCount - 1);
                if (d && d->kind == TYPE_DERIVATION_ARRAY && d->as.array.isFlexible) {
                    outInfo->isFlexBase = true;
                    uint64_t size = 0;
                    uint32_t al = 0;
                    ParsedType elem = parsedTypeArrayElementType(fieldParsed);
                    cg_size_align_for_type(ctx, &elem, fieldType, &size, &al);
                    parsedTypeFree(&elem);
                    outInfo->flexElemSize = size ? size : 1;
                }
            }
            if (outInfo && !outInfo->isFlexBase && lay && !lay->isBitfield && lay->storageUnitBytes == 0) {
                outInfo->isFlexBase = true;
                uint64_t size = 0;
                uint32_t al = 0;
                if (fieldParsed && parsedTypeIsDirectArray(fieldParsed)) {
                    ParsedType elem = parsedTypeArrayElementType(fieldParsed);
                    cg_size_align_for_type(ctx, &elem, fieldType, &size, &al);
                    parsedTypeFree(&elem);
                }
                if (size == 0) {
                    cg_size_align_for_type(ctx, NULL, fieldType, &size, &al);
                }
                outInfo->flexElemSize = size ? size : 1;
            }
            if (havePointed) {
                parsedTypeFree(&pointed);
            }
            if (haveArrayElem) {
                parsedTypeFree(&arrayElem);
            }
            return true;
        }
        case AST_COMPOUND_LITERAL: {
            LLVMTypeRef litType = NULL;
            LLVMValueRef ptr = cg_emit_compound_literal_pointer(ctx, target, &litType);
            if (!ptr) {
                return false;
            }
            *outPtr = ptr;
            *outType = litType ? litType : LLVMInt32TypeInContext(ctx->llvmContext);
            if (outParsedType) {
                *outParsedType = &target->compoundLiteral.literalType;
            }
            return true;
        }
        default:
            break;
    }
    return false;
}

LLVMTypeRef codegenStructDefinition(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_STRUCT_DEFINITION && node->type != AST_UNION_DEFINITION) {
        fprintf(stderr, "Error: Invalid node type for codegenStructDefinition\n");
        return NULL;
    }

    CG_DEBUG("[CG] Struct/Union definition node\n");
    ASTNode* nameNode = node->structDef.structName;
    const char* structName = (nameNode && nameNode->type == AST_IDENTIFIER) ? nameNode->valueNode.value : NULL;
    bool isUnion = (node->type == AST_UNION_DEFINITION);
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    CGStructLLVMInfo* semanticInfo = NULL;
    bool semanticInfoMatchesNode = false;
    bool forceScopedTypeName = false;
    if (cache) {
        semanticInfo = cg_type_cache_get_struct_by_definition(cache, node);
        if (semanticInfo) {
            semanticInfoMatchesNode = true;
        } else if (structName && structName[0]) {
            CGStructLLVMInfo* byName = cg_type_cache_get_struct_info(cache, structName);
            if (byName) {
                if (byName->definition == node) {
                    semanticInfo = byName;
                    semanticInfoMatchesNode = true;
                } else {
                    // Same spelling, different definition (block-scope shadowing).
                    forceScopedTypeName = true;
                }
            }
        }
    }

    LLVMTypeRef structType = NULL;
    if (semanticInfo && semanticInfo->llvmType) {
        structType = semanticInfo->llvmType;
    }

    if (!structType) {
        if (!forceScopedTypeName && structName && structName[0]) {
            structType = ensureStructLLVMTypeByName(ctx, structName, isUnion);
        } else {
            char anonName[96];
            snprintf(anonName,
                     sizeof(anonName),
                     isUnion ? "scoped.union.%p" : "scoped.struct.%p",
                     (void*)node);
            structType = cg_context_lookup_named_type(ctx, anonName);
            if (!structType) {
                structType = LLVMStructCreateNamed(ctx->llvmContext, anonName);
                cg_context_cache_named_type(ctx, anonName, structType);
            }
            structName = anonName;
        }
    }

    if (semanticInfo && semanticInfoMatchesNode && !semanticInfo->llvmType) {
        semanticInfo->llvmType = structType;
    }

    if (!LLVMIsOpaqueStruct(structType)) {
        return structType;
    }

    size_t totalFields = 0;
    for (size_t i = 0; i < node->structDef.fieldCount; ++i) {
        ASTNode* fieldNode = node->structDef.fields[i];
        if (!fieldNode) continue;
        if (fieldNode->type == AST_VARIABLE_DECLARATION) {
            totalFields += fieldNode->varDecl.varCount;
        }
    }

    if (totalFields == 0) {
        LLVMStructSetBody(structType, NULL, 0, 0);
        recordStructInfo(ctx, structName, isUnion, NULL, 0, structType);
        return structType;
    }

    LLVMTypeRef* fieldTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * totalFields);
    StructFieldInfo* infos = (StructFieldInfo*)calloc(totalFields, sizeof(StructFieldInfo));
    if (!fieldTypes || !infos) {
        free(fieldTypes);
        free(infos);
        return NULL;
    }

    size_t fieldIndex = 0;
    for (size_t i = 0; i < node->structDef.fieldCount; ++i) {
        ASTNode* fieldNode = node->structDef.fields[i];
        if (!fieldNode) continue;
        if (fieldNode->type != AST_VARIABLE_DECLARATION) {
            fprintf(stderr, "Warning: Unsupported struct member node type %d\n", fieldNode->type);
            continue;
        }

        for (size_t v = 0; v < fieldNode->varDecl.varCount; ++v) {
            if (fieldIndex >= totalFields) break;
            ASTNode* nameNodeField = fieldNode->varDecl.varNames[v];
            const char* fname = (nameNodeField && nameNodeField->type == AST_IDENTIFIER)
                ? nameNodeField->valueNode.value
                : NULL;

            const ParsedType* parsed = astVarDeclTypeAt(fieldNode, v);
            LLVMTypeRef memberType = cg_type_from_parsed(ctx, parsed);
            if (!memberType || LLVMGetTypeKind(memberType) == LLVMVoidTypeKind) {
                memberType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            char* memberTypeStr = LLVMPrintTypeToString(memberType);
            CG_DEBUG("[CG] Struct member type=%s\n", memberTypeStr ? memberTypeStr : "<null>");
            if (memberTypeStr) LLVMDisposeMessage(memberTypeStr);

            fieldTypes[fieldIndex] = memberType;
            infos[fieldIndex].name = fname ? strdup(fname) : NULL;
            infos[fieldIndex].index = isUnion ? 0 : (unsigned)fieldIndex;
            infos[fieldIndex].type = memberType;
            infos[fieldIndex].parsedType = parsed ? *parsed : fieldNode->varDecl.declaredType;
            fieldIndex++;
        }
    }

    if (isUnion) {
        LLVMModuleRef module = cg_context_get_module(ctx);
        LLVMTargetDataRef td = module ? LLVMGetModuleDataLayout(module) : NULL;
        LLVMTypeRef maxAlignTy = NULL;
        uint64_t maxSize = 0;
        uint32_t maxAlign = 1;
        uint64_t maxAlignTySize = 0;

        for (size_t i = 0; i < fieldIndex; ++i) {
            LLVMTypeRef memberTy = fieldTypes[i];
            if (!memberTy || LLVMGetTypeKind(memberTy) == LLVMVoidTypeKind) {
                memberTy = LLVMInt8TypeInContext(ctx->llvmContext);
            }
            uint64_t sz = td ? LLVMABISizeOfType(td, memberTy) : 0;
            uint32_t al = td ? (uint32_t)LLVMABIAlignmentOfType(td, memberTy) : 1;
            if (al == 0) al = 1;
            if (sz > maxSize) maxSize = sz;
            if (!maxAlignTy || al > maxAlign || (al == maxAlign && sz > maxAlignTySize)) {
                maxAlignTy = memberTy;
                maxAlign = al;
                maxAlignTySize = sz;
            }
        }

        if (!maxAlignTy) {
            maxAlignTy = LLVMInt8TypeInContext(ctx->llvmContext);
            maxAlign = 1;
            maxSize = 1;
            maxAlignTySize = 1;
        }

        uint64_t finalSize = maxSize;
        if (maxAlign > 1) {
            uint64_t rem = finalSize % (uint64_t)maxAlign;
            if (rem != 0) finalSize += ((uint64_t)maxAlign - rem);
        }
        if (finalSize == 0) finalSize = 1;

        if (maxAlignTySize >= finalSize) {
            LLVMStructSetBody(structType, &maxAlignTy, 1, 0);
        } else {
            uint64_t tailBytes = finalSize - maxAlignTySize;
            LLVMTypeRef members[2];
            members[0] = maxAlignTy;
            members[1] = LLVMArrayType(LLVMInt8TypeInContext(ctx->llvmContext),
                                       (unsigned)tailBytes);
            LLVMStructSetBody(structType, members, 2, 0);
        }
    } else {
        LLVMStructSetBody(structType, fieldTypes, (unsigned)fieldIndex, 0);
    }
    recordStructInfo(ctx, structName, isUnion, infos, fieldIndex, structType);
    free(fieldTypes);
    return structType;
}


LLVMValueRef codegenStructFieldAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_STRUCT_FIELD_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenStructFieldAccess\n");
        return NULL;
    }
    LLVMValueRef fieldPtr = NULL;
    LLVMTypeRef fieldType = NULL;
    const ParsedType* fieldParsed = NULL;
    CGLValueInfo info;
    if (!codegenLValue(ctx, node, &fieldPtr, &fieldType, &fieldParsed, &info)) {
        fprintf(stderr, "Error: Unknown struct field %s\n", node->structFieldAccess.fieldName);
        return NULL;
    }
    if (info.isBitfield) {
        unsigned width = info.layout.widthBits ? (unsigned)info.layout.widthBits : LLVMGetIntTypeWidth(info.storageType);
        if (width == 0) width = LLVMGetIntTypeWidth(info.storageType);
        LLVMTypeRef resultTy = LLVMIntTypeInContext(ctx->llvmContext, width);
        return cg_structs_load_bitfield(ctx, &info, resultTy);
    }
    if (fieldType && LLVMGetTypeKind(fieldType) == LLVMArrayTypeKind) {
        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef idxs[2] = { zero, zero };
        return LLVMBuildGEP2(ctx->builder, fieldType, fieldPtr, idxs, 2, "struct_field_decay");
    }
    if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
        fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return LLVMBuildLoad2(ctx->builder, fieldType, fieldPtr, "fieldLoad");
}
