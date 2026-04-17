// SPDX-License-Identifier: Apache-2.0

#include "codegen_expr_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LLVMValueRef codegenArrayAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_ARRAY_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenArrayAccess\n");
        return NULL;
    }
    const char* dbgRun = getenv("DEBUG_RUN");
    if (dbgRun) fprintf(stderr, "[CG] array access begin\n");

    LLVMValueRef arrayPtr = NULL;
    LLVMTypeRef arrayType = NULL;
    const ParsedType* arrayParsed = NULL;
    CGLValueInfo arrLValInfo;
    bool haveLVal =
        codegenLValue(ctx, node->arrayAccess.array, &arrayPtr, &arrayType, &arrayParsed, &arrLValInfo);
    if (!haveLVal) {
        arrayPtr = codegenNode(ctx, node->arrayAccess.array);
    }
    /* For pointer variables (not true arrays), arrayPtr currently holds the address of the
       pointer object (e.g., an alloca of `int*`). Load the pointer value so we index the
       pointee rather than the stack slot. */
    bool baseIsDirectArray = (arrayParsed && parsedTypeIsDirectArray(arrayParsed)) ||
                             (arrayType && LLVMGetTypeKind(arrayType) == LLVMArrayTypeKind);
    bool baseNeedsLoad = haveLVal && arrayPtr && !baseIsDirectArray;
    if (baseNeedsLoad) {
        LLVMTypeRef loadTy = arrayType;
        if (!loadTy) {
            LLVMTypeRef elem = NULL;
            if (arrayPtr && LLVMGetTypeKind(LLVMTypeOf(arrayPtr)) == LLVMPointerTypeKind) {
                LLVMTypeRef ptrTy = LLVMTypeOf(arrayPtr);
                if (!LLVMPointerTypeIsOpaque(ptrTy)) {
                    elem = LLVMGetElementType(ptrTy);
                }
            }
            loadTy = elem ? elem : LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
        }
        arrayPtr = LLVMBuildLoad2(ctx->builder, loadTy, arrayPtr, "array.ptr.load");
        arrayType = loadTy;
    }
    LLVMValueRef index = codegenNode(ctx, node->arrayAccess.index);
    if (!arrayPtr || !index) {
        fprintf(stderr, "Error: Array access failed\n");
        return NULL;
    }
    if (dbgRun) {
        char* atStr = LLVMPrintTypeToString(LLVMTypeOf(arrayPtr));
        fprintf(stderr, "[CG] arrayPtr LLVM type=%s\n", atStr ? atStr : "<null>");
        if (atStr) LLVMDisposeMessage(atStr);
    }
    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    if (!arrayParsed) {
        arrayParsed = cg_resolve_expression_type(ctx, node->arrayAccess.array);
    }
    const ParsedType* refinedCallParsed =
        cg_refine_function_call_result_type(ctx, node->arrayAccess.array);
    if (refinedCallParsed) {
        arrayParsed = refinedCallParsed;
    }
    if (dbgRun && arrayParsed) {
        fprintf(stderr, "[CG] array parsed kind=%d prim=%d name=%s ptrDepth=%d derivs=%zu\n",
                arrayParsed->kind,
                arrayParsed->primitiveType,
                arrayParsed->userTypeName ? arrayParsed->userTypeName : "<none>",
                arrayParsed->pointerDepth,
                arrayParsed->derivationCount);
    }

    if (arrayParsed && parsedTypeIsDirectArray(arrayParsed) && parsedTypeHasVLA(arrayParsed)) {
        size_t arrayDims = 0;
        for (size_t i = 0; i < arrayParsed->derivationCount; ++i) {
            const TypeDerivation* deriv = parsedTypeGetDerivation(arrayParsed, i);
            if (deriv && deriv->kind == TYPE_DERIVATION_ARRAY) {
                arrayDims++;
            }
        }
        LLVMValueRef stride = LLVMConstInt(intptrTy, 1, 0);
        for (size_t i = 1; i < arrayParsed->derivationCount; ++i) {
            const TypeDerivation* deriv = parsedTypeGetDerivation(arrayParsed, i);
            if (!deriv || deriv->kind != TYPE_DERIVATION_ARRAY) continue;
            LLVMValueRef dim = NULL;
            if (!deriv->as.array.isVLA && deriv->as.array.hasConstantSize && deriv->as.array.constantSize > 0) {
                dim = LLVMConstInt(intptrTy, (unsigned long long)deriv->as.array.constantSize, 0);
            } else if (deriv->as.array.sizeExpr) {
                LLVMValueRef evaluated = codegenNode(ctx, deriv->as.array.sizeExpr);
                dim = ensureIntegerLike(ctx, evaluated);
            }
            if (!dim) continue;
            stride = LLVMBuildMul(ctx->builder, stride, dim, "vla.stride");
        }

        LLVMValueRef offset = ensureIntegerLike(ctx, index);
        if (!offset) return NULL;
        if (arrayDims > 1) {
            offset = LLVMBuildMul(ctx->builder, offset, stride, "vla.offset");
        }

        LLVMTypeRef elemType = vlaInnermostElementLLVM(ctx, arrayParsed);
        LLVMValueRef elementPtr = LLVMBuildGEP2(ctx->builder, elemType, arrayPtr, &offset, 1, "vla.elem.ptr");

        bool hasMoreDims = arrayDims > 1;
        if (hasMoreDims) {
            return elementPtr;
        }
        LLVMValueRef loadVal = LLVMBuildLoad2(ctx->builder, elemType, elementPtr, "arrayLoad");
        return loadVal;
    }
    char* arrTyStr = LLVMPrintTypeToString(LLVMTypeOf(arrayPtr));
    CG_DEBUG("[CG] Array access base type: %s\n", arrTyStr ? arrTyStr : "<null>");
    if (arrTyStr) LLVMDisposeMessage(arrTyStr);
    LLVMTypeRef aggregateHint = NULL;
    if (arrayParsed && parsedTypeIsDirectArray(arrayParsed)) {
        aggregateHint = cg_type_from_parsed(ctx, arrayParsed);
    } else if (arrayType && LLVMGetTypeKind(arrayType) == LLVMArrayTypeKind) {
        aggregateHint = arrayType;
    }
    LLVMTypeRef elementHint = NULL;
    /* Prefer concrete LLVM array/pointer element hints over semantic fallbacks.
       This avoids stale parsed-type hints selecting unrelated aggregate element
       types on member-array expressions (e.g. ptr->arr[idx]). */
    if (arrayType && LLVMGetTypeKind(arrayType) == LLVMArrayTypeKind) {
        elementHint = LLVMGetElementType(arrayType);
    } else if (arrayType &&
               LLVMGetTypeKind(arrayType) == LLVMPointerTypeKind &&
               !LLVMPointerTypeIsOpaque(arrayType)) {
        LLVMTypeRef elem = LLVMGetElementType(arrayType);
        if (elem) {
            elementHint = elem;
        }
    }
    if (!elementHint) {
        elementHint = cg_element_type_hint_from_parsed(ctx, arrayParsed);
    }
    LLVMTypeRef derivedElementType = NULL;
    LLVMValueRef elementPtr = buildArrayElementPointer(ctx,
                                                       arrayPtr,
                                                       index,
                                                       arrayParsed,
                                                       aggregateHint,
                                                       elementHint,
                                                       &derivedElementType);
    if (!elementPtr) {
        fprintf(stderr, "Error: Failed to compute array element pointer\n");
        return NULL;
    }
    if (arrayParsed && cg_parsed_type_is_pointerish(arrayParsed)) {
        ParsedType pointed = parsedTypePointerTargetType(arrayParsed);
        bool pointerToVLAArray =
            pointed.kind != TYPE_INVALID &&
            parsedTypeIsDirectArray(&pointed) &&
            parsedTypeHasVLA(&pointed);
        if (getenv("DEBUG_RUN")) {
            fprintf(stderr,
                    "[CG] ptr-array access pointerToVLA=%d basePtrDepth=%d baseDerivs=%zu pointedDerivs=%zu\n",
                    pointerToVLAArray ? 1 : 0,
                    arrayParsed->pointerDepth,
                    arrayParsed->derivationCount,
                    pointed.derivationCount);
        }
        parsedTypeFree(&pointed);
        if (pointerToVLAArray) {
            return elementPtr;
        }
    }
    CG_DEBUG("[CG] Array element pointer computed\n");
    LLVMTypeRef ptrToElemType = LLVMTypeOf(elementPtr);
    char* ptrElemStr = ptrToElemType ? LLVMPrintTypeToString(ptrToElemType) : NULL;
    CG_DEBUG("[CG] Array element pointer LLVM type=%s\n", ptrElemStr ? ptrElemStr : "<null>");
    if (ptrElemStr) LLVMDisposeMessage(ptrElemStr);
    if (dbgRun) {
        char* hintStr = elementHint ? LLVMPrintTypeToString(elementHint) : NULL;
        char* derivedStr = derivedElementType ? LLVMPrintTypeToString(derivedElementType) : NULL;
        fprintf(stderr, "[CG] elementHint=%s derived=%s\n",
                hintStr ? hintStr : "<null>",
                derivedStr ? derivedStr : "<null>");
        if (hintStr) LLVMDisposeMessage(hintStr);
        if (derivedStr) LLVMDisposeMessage(derivedStr);
    }

    LLVMTypeRef elementType = derivedElementType ? derivedElementType : elementHint;
    if (!elementType) {
        elementType = cg_dereference_ptr_type(ctx, ptrToElemType, "array access");
    }
    if (!elementType || LLVMGetTypeKind(elementType) == LLVMVoidTypeKind) {
        if (LLVMGetTypeKind(ptrToElemType) == LLVMPointerTypeKind && LLVMGetElementType(ptrToElemType) == NULL) {
            fprintf(stderr, "Error: opaque pointer array access without element hint\n");
            return NULL;
        }
        elementType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    if (dbgRun) {
        char* etStr = LLVMPrintTypeToString(elementType);
        fprintf(stderr, "[CG] array elem type %s\n", etStr ? etStr : "<null>");
        if (etStr) LLVMDisposeMessage(etStr);
    }
    LLVMValueRef typedElementPtr = elementPtr;
    LLVMTypeRef loadTy = elementType ? elementType : LLVMInt8TypeInContext(ctx->llvmContext);
    if (loadTy && LLVMGetTypeKind(loadTy) == LLVMFunctionTypeKind) {
        loadTy = LLVMPointerType(loadTy, 0);
    }
    if (!typedElementPtr || !LLVMTypeOf(typedElementPtr) ||
        LLVMGetTypeKind(LLVMTypeOf(typedElementPtr)) != LLVMPointerTypeKind) {
        fprintf(stderr, "Error: invalid pointer for array load\n");
        return NULL;
    }
    if (loadTy && LLVMGetTypeKind(loadTy) == LLVMArrayTypeKind) {
        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef idxs[2] = { zero, zero };
        return LLVMBuildGEP2(ctx->builder, loadTy, typedElementPtr, idxs, 2, "array.elem.decay");
    }
    if (dbgRun) fprintf(stderr, "[CG] array load begin\n");
    LLVMTypeRef expectedPtrTy = LLVMPointerType(loadTy, 0);
    if (LLVMTypeOf(typedElementPtr) != expectedPtrTy) {
        typedElementPtr = LLVMBuildBitCast(ctx->builder, typedElementPtr, expectedPtrTy, "array.elem.cast");
    }
    LLVMValueRef loadVal = LLVMBuildLoad2(ctx->builder, loadTy, typedElementPtr, "arrayLoad");
    CG_DEBUG("[CG] Array element load complete\n");
    if (dbgRun) fprintf(stderr, "[CG] array access end\n");
    return loadVal;
}

LLVMValueRef codegenPointerAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_POINTER_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenPointerAccess\n");
        return NULL;
    }
    LLVMValueRef fieldPtr = NULL;
    LLVMTypeRef fieldType = NULL;
    const ParsedType* fieldParsed = NULL;
    CGLValueInfo info;
    if (!codegenLValue(ctx, node, &fieldPtr, &fieldType, &fieldParsed, &info)) {
        const char* fieldName = node->memberAccess.field ? node->memberAccess.field : "<unknown>";
        fprintf(stderr,
                "Error: Unknown field in pointer access: %s at line %d\n",
                fieldName,
                node->line);
        return NULL;
    }
    if (info.isBitfield) {
        unsigned width = info.layout.widthBits ? (unsigned)info.layout.widthBits : LLVMGetIntTypeWidth(info.storageType);
        if (width == 0) width = LLVMGetIntTypeWidth(info.storageType);
        LLVMTypeRef resultTy = LLVMIntTypeInContext(ctx->llvmContext, width);
        return cg_load_bitfield(ctx, &info, resultTy);
    }
    if (fieldType && LLVMGetTypeKind(fieldType) == LLVMArrayTypeKind) {
        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef idxs[2] = { zero, zero };
        return LLVMBuildGEP2(ctx->builder, fieldType, fieldPtr, idxs, 2, "ptr_field_decay");
    }
    if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
        fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return LLVMBuildLoad2(ctx->builder, fieldType, fieldPtr, "ptr_field");
}

LLVMValueRef codegenDotAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_DOT_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenDotAccess\n");
        return NULL;
    }
    LLVMValueRef fieldPtr = NULL;
    LLVMTypeRef fieldType = NULL;
    const ParsedType* fieldParsed = NULL;
    CGLValueInfo info;
    if (!codegenLValue(ctx, node, &fieldPtr, &fieldType, &fieldParsed, &info)) {
        const char* fieldName = node->memberAccess.field ? node->memberAccess.field : "<unknown>";
        fprintf(stderr, "Error: Unknown field in dot access: %s\n", fieldName);
        return NULL;
    }
    if (info.isBitfield) {
        unsigned width = info.layout.widthBits ? (unsigned)info.layout.widthBits : LLVMGetIntTypeWidth(info.storageType);
        if (width == 0) width = LLVMGetIntTypeWidth(info.storageType);
        LLVMTypeRef resultTy = LLVMIntTypeInContext(ctx->llvmContext, width);
        LLVMValueRef result = cg_load_bitfield(ctx, &info, resultTy);
        CG_DEBUG("[CG] Dot access bitfield load complete\n");
        return result;
    }
    if (fieldType && LLVMGetTypeKind(fieldType) == LLVMArrayTypeKind) {
        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef idxs[2] = { zero, zero };
        return LLVMBuildGEP2(ctx->builder, fieldType, fieldPtr, idxs, 2, "dot_field_decay");
    }
    if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
        fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return LLVMBuildLoad2(ctx->builder, fieldType, fieldPtr, "dot_field");
}

LLVMValueRef codegenPointerDereference(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_POINTER_DEREFERENCE) {
        fprintf(stderr, "Error: Invalid node type for codegenPointerDereference\n");
        return NULL;
    }

    LLVMValueRef pointer = codegenNode(ctx, node->pointerDeref.pointer);
    if (!pointer) {
        fprintf(stderr, "Error: Failed to generate pointer dereference\n");
        return NULL;
    }
    LLVMTypeRef elemType = NULL;
    /* Prefer semantic type information if available. */
    const ParsedType* ptrParsed = cg_resolve_expression_type(ctx, node->pointerDeref.pointer);
    if (ptrParsed && ptrParsed->pointerDepth > 0) {
        elemType = cg_element_type_from_pointer_parsed(ctx, ptrParsed);
        if (elemType && LLVMGetTypeKind(elemType) == LLVMPointerTypeKind) {
            ParsedType base = parsedTypeClone(ptrParsed);
            if (base.pointerDepth > 0) {
                base.pointerDepth -= 1;
            }
            for (ssize_t i = (ssize_t)base.derivationCount - 1; i >= 0; --i) {
                if (base.derivations[i].kind == TYPE_DERIVATION_POINTER) {
                    for (size_t j = (size_t)i; j + 1 < base.derivationCount; ++j) {
                        base.derivations[j] = base.derivations[j + 1];
                    }
                    base.derivationCount -= 1;
                    break;
                }
            }
            LLVMTypeRef forced = cg_type_from_parsed(ctx, &base);
            parsedTypeFree(&base);
            if (forced && LLVMGetTypeKind(forced) != LLVMPointerTypeKind && LLVMGetTypeKind(forced) != LLVMVoidTypeKind) {
                elemType = forced;
            }
        }
    }
    /* If the operand is an identifier, try to use its stored element type to avoid opaque-pointer ambiguity. */
    if (!elemType && node->pointerDeref.pointer && node->pointerDeref.pointer->type == AST_IDENTIFIER) {
        const char* name = node->pointerDeref.pointer->valueNode.value;
        NamedValue* entry = cg_scope_lookup(ctx->currentScope, name);
        if (entry && entry->elementType) {
            elemType = entry->elementType;
        } else if (entry && entry->parsedType && entry->parsedType->pointerDepth > 0) {
            elemType = cg_element_type_from_pointer_parsed(ctx, entry->parsedType);
        }
    }
    if (!elemType) {
        elemType = cg_dereference_ptr_type(ctx, LLVMTypeOf(pointer), "pointer dereference");
    }
    if (!elemType || LLVMGetTypeKind(elemType) == LLVMVoidTypeKind) {
        elemType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return LLVMBuildLoad2(ctx->builder, elemType, pointer, "ptrLoad");
}

static LLVMValueRef cg_materialize_compound_literal(CodegenContext* ctx,
                                                    ASTNode* node,
                                                    LLVMTypeRef* outLiteralType) {
    if (!ctx || !node || node->type != AST_COMPOUND_LITERAL) {
        return NULL;
    }
    LLVMTypeRef literalType = cg_type_from_parsed(ctx, &node->compoundLiteral.literalType);
    if (!literalType || LLVMGetTypeKind(literalType) == LLVMVoidTypeKind) {
        literalType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    if (outLiteralType) {
        *outLiteralType = literalType;
    }

    if (node->compoundLiteral.isStaticStorage) {
        LLVMValueRef constInit = cg_build_const_initializer(ctx,
                                                            node,
                                                            literalType,
                                                            &node->compoundLiteral.literalType);
        if (constInit) {
            static unsigned counter = 0;
            char name[64];
            snprintf(name, sizeof(name), ".compound.static.%u", counter++);
            LLVMValueRef global = LLVMAddGlobal(ctx->module, literalType, name);
            LLVMSetLinkage(global, LLVMPrivateLinkage);
            LLVMSetGlobalConstant(global, 1);
            LLVMSetInitializer(global, constInit);
            LLVMSetUnnamedAddr(global, LLVMGlobalUnnamedAddr);
            return global;
        }
    }

    LLVMValueRef tmp = cg_build_entry_alloca(ctx, literalType, "compound.tmp");
    if (!cg_store_compound_literal_into_ptr(ctx,
                                            tmp,
                                            literalType,
                                            &node->compoundLiteral.literalType,
                                            node)) {
        return NULL;
    }
    return tmp;
}

LLVMValueRef cg_emit_compound_literal_pointer(CodegenContext* ctx, ASTNode* node, LLVMTypeRef* outType) {
    LLVMTypeRef literalType = NULL;
    LLVMValueRef ptr = cg_materialize_compound_literal(ctx, node, &literalType);
    if (!ptr) return NULL;
    if (outType) {
        *outType = literalType;
    }
    return ptr;
}

LLVMValueRef codegenCompoundLiteral(CodegenContext* ctx, ASTNode* node) {
    if (!node || node->type != AST_COMPOUND_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenCompoundLiteral\n");
        return NULL;
    }

    LLVMTypeRef literalType = NULL;
    LLVMValueRef ptr = cg_materialize_compound_literal(ctx, node, &literalType);
    if (!ptr || !literalType || LLVMGetTypeKind(literalType) == LLVMVoidTypeKind) {
        fprintf(stderr, "Error: Failed to emit compound literal\n");
        return NULL;
    }

    return LLVMBuildLoad2(ctx->builder, literalType, ptr, "compound.value");
}

LLVMValueRef codegenIdentifier(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_IDENTIFIER) {
        fprintf(stderr, "Error: Invalid node type for codegenIdentifier\n");
        return NULL;
    }

    if (node->valueNode.value && strcmp(node->valueNode.value, "__func__") == 0) {
        const char* fnName = ctx->currentFunctionName ? ctx->currentFunctionName : "";
        return LLVMBuildGlobalStringPtr(ctx->builder, fnName, "__func__");
    }

    NamedValue* entry = cg_scope_lookup(ctx->currentScope, node->valueNode.value);
    if (entry) {
        bool entryIsArray = entry->parsedType && parsedTypeIsDirectArray(entry->parsedType);
        if (entryIsArray) {
            LLVMTypeRef basePtrTy = LLVMTypeOf(entry->value);
            if (basePtrTy &&
                LLVMGetTypeKind(basePtrTy) == LLVMPointerTypeKind &&
                !LLVMPointerTypeIsOpaque(basePtrTy)) {
                LLVMTypeRef pointee = LLVMGetElementType(basePtrTy);
                if (pointee && LLVMGetTypeKind(pointee) == LLVMArrayTypeKind) {
                    LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
                    LLVMValueRef idxs[2] = { zero, zero };
                    return LLVMBuildGEP2(ctx->builder, pointee, entry->value, idxs, 2, "array.decay");
                }
            }
            return entry->value;
        }
        if (entry->addressOnly) {
            return entry->value;
        }

        LLVMTypeRef loadType = entry->type;
        if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
            loadType = cg_dereference_ptr_type(ctx, LLVMTypeOf(entry->value), "identifier load");
        }
        if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
            loadType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        return cg_build_load(ctx, loadType, entry->value, node->valueNode.value, entry->parsedType);
    }

    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (model) {
        const Symbol* sym = semanticModelLookupGlobal(model, node->valueNode.value);
        if (sym &&
            sym->kind == SYMBOL_VARIABLE &&
            sym->hasConstValue &&
            cg_is_builtin_bool_literal_name(sym->name)) {
            LLVMTypeRef boolConstType = cg_type_from_parsed(ctx, &sym->type);
            if (!boolConstType || LLVMGetTypeKind(boolConstType) != LLVMIntegerTypeKind) {
                boolConstType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            return LLVMConstInt(boolConstType, (unsigned long long)sym->constValue, 0);
        }
        if (sym && sym->kind == SYMBOL_ENUM && sym->hasConstValue) {
            LLVMTypeRef enumType = cg_type_from_parsed(ctx, &sym->type);
            if (!enumType || LLVMGetTypeKind(enumType) != LLVMIntegerTypeKind) {
                enumType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            return LLVMConstInt(enumType, (unsigned long long)sym->constValue, 1);
        }
    }

    LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, node->valueNode.value);
    if (!global) {
        LLVMValueRef fn = LLVMGetNamedFunction(ctx->module, node->valueNode.value);
        if (fn) {
            return fn;
        }
        fprintf(stderr, "Error: Undefined variable %s\n", node->valueNode.value);
        return NULL;
    }

    LLVMTypeRef loadType = cg_dereference_ptr_type(ctx, LLVMTypeOf(global), "global load");
    if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
        loadType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    const ParsedType* parsedType = NULL;
    if (model) {
        const Symbol* sym = semanticModelLookupGlobal(model, node->valueNode.value);
        if (sym) {
            parsedType = &sym->type;
        }
    }
    cg_scope_insert(ctx->currentScope,
                    node->valueNode.value,
                    global,
                    loadType,
                    true,
                    false,
                    NULL,
                    parsedType);
    return cg_build_load(ctx, loadType, global, node->valueNode.value, parsedType);
}
