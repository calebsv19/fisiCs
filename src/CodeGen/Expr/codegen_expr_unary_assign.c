// SPDX-License-Identifier: Apache-2.0

#include "codegen_expr_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LLVMValueRef codegenUnaryExpression(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_UNARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenUnaryExpression\n");
        return NULL;
    }

    if (node->expr.op && (strcmp(node->expr.op, "++") == 0 || strcmp(node->expr.op, "--") == 0)) {
        bool isIncrement = (node->expr.op[0] == '+');
        bool isPostfix = node->expr.isPostfix;

        LLVMValueRef targetPtr = NULL;
        LLVMTypeRef targetType = NULL;
        const ParsedType* targetParsed = NULL;
        CGLValueInfo lvalInfo;
        if (!codegenLValue(ctx, node->expr.left, &targetPtr, &targetType, &targetParsed, &lvalInfo)) {
            fprintf(stderr, "Error: ++/-- operand must be assignable\n");
            return NULL;
        }

        LLVMValueRef current = NULL;
        if (lvalInfo.isBitfield) {
            unsigned width = lvalInfo.layout.widthBits ? (unsigned)lvalInfo.layout.widthBits : LLVMGetIntTypeWidth(lvalInfo.storageType);
            if (width == 0) width = LLVMGetIntTypeWidth(lvalInfo.storageType);
            LLVMTypeRef fieldType = LLVMIntTypeInContext(ctx->llvmContext, width);
            current = cg_load_bitfield(ctx, &lvalInfo, fieldType);
        } else {
            current = cg_build_load(ctx, targetType, targetPtr, "unary.load", targetParsed);
        }
        if (!current) {
            fprintf(stderr, "Error: Failed to load ++/-- operand\n");
            return NULL;
        }
        if (LLVMGetTypeKind(targetType) == LLVMPointerTypeKind) {
            LLVMValueRef one = LLVMConstInt(cg_get_intptr_type(ctx), 1, 0);
            LLVMValueRef updated = cg_build_pointer_offset(ctx, current, one, targetParsed, NULL, !isIncrement);
            if (!updated) {
                return NULL;
            }
            cg_build_store(ctx, updated, targetPtr, targetParsed);
            return isPostfix ? current : updated;
        }

        if (!targetType || LLVMGetTypeKind(targetType) != LLVMIntegerTypeKind) {
            targetType = LLVMInt32TypeInContext(ctx->llvmContext);
            current = cg_cast_value(ctx, current, targetType, targetParsed, NULL, "unary.cast");
        }
        LLVMValueRef one = LLVMConstInt(targetType, 1, 0);
        LLVMValueRef updated = isIncrement
            ? LLVMBuildAdd(ctx->builder, current, one, "unary.inc")
            : LLVMBuildSub(ctx->builder, current, one, "unary.dec");

        if (lvalInfo.isBitfield) {
            if (!cg_store_bitfield(ctx, &lvalInfo, updated)) return NULL;
        } else {
            cg_build_store(ctx, updated, targetPtr, targetParsed);
        }
        return isPostfix ? current : updated;
    }

    if (node->expr.op && strcmp(node->expr.op, "&") == 0) {
        LLVMValueRef addrPtr = NULL;
        LLVMTypeRef addrType = NULL;
        CGLValueInfo tmp;
        if (!codegenLValue(ctx, node->expr.left, &addrPtr, &addrType, NULL, &tmp)) {
            if (node->expr.left && node->expr.left->type == AST_IDENTIFIER) {
                const SemanticModel* model = cg_context_get_semantic_model(ctx);
                if (model) {
                    const Symbol* sym = semanticModelLookupGlobal(model, node->expr.left->valueNode.value);
                    if (sym && sym->kind == SYMBOL_FUNCTION) {
                        LLVMValueRef fn = LLVMGetNamedFunction(ctx->module, node->expr.left->valueNode.value);
                        if (fn) {
                            return fn;
                        }
                    }
                }
            }
            fprintf(stderr, "Error: Address-of requires an lvalue\n");
            return NULL;
        }
        if (tmp.isBitfield) {
            fprintf(stderr, "Error: Address-of bitfield is invalid\n");
            return NULL;
        }
        return addrPtr;
    }

    const ParsedType* operandParsed = cg_resolve_expression_type(ctx, node->expr.left);
    ParsedType operandParsedCopy = parsedTypeClone(operandParsed);
    ParsedType derivedPointerParsed = parsedTypeClone(NULL);
    bool hasDerivedPointerParsed = false;
    const ParsedType* stableParsed = operandParsedCopy.kind != TYPE_INVALID ? &operandParsedCopy : operandParsed;
    LLVMValueRef operand = codegenNode(ctx, node->expr.left);
    if (!operand) {
        parsedTypeFree(&operandParsedCopy);
        fprintf(stderr, "Error: Failed to generate operand for unary expression\n");
        return NULL;
    }

    if (strcmp(node->expr.op, "+") == 0) {
        parsedTypeFree(&operandParsedCopy);
        if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
        return operand;
    } else if (strcmp(node->expr.op, "-") == 0) {
        if (cg_parsed_type_is_complex_value(stableParsed) &&
            LLVMGetTypeKind(LLVMTypeOf(operand)) == LLVMStructTypeKind) {
            LLVMTypeRef complexType = LLVMTypeOf(operand);
            LLVMValueRef realPart = LLVMBuildExtractValue(ctx->builder, operand, 0, "complex.neg.r");
            LLVMValueRef imagPart = LLVMBuildExtractValue(ctx->builder, operand, 1, "complex.neg.i");
            LLVMValueRef realNeg = LLVMBuildFNeg(ctx->builder, realPart, "complex.neg.rn");
            LLVMValueRef imagNeg = LLVMBuildFNeg(ctx->builder, imagPart, "complex.neg.in");
            parsedTypeFree(&operandParsedCopy);
            if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
            return cg_build_complex_value(ctx, realNeg, imagNeg, complexType, "complex.neg");
        }
        LLVMTypeRef operandType = LLVMTypeOf(operand);
        parsedTypeFree(&operandParsedCopy);
        if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
        if (operandType && cg_is_float_type(operandType)) {
            return LLVMBuildFNeg(ctx->builder, operand, "fnegtmp");
        }
        return LLVMBuildNeg(ctx->builder, operand, "negtmp");
    } else if (strcmp(node->expr.op, "~") == 0) {
        LLVMTypeRef operandType = LLVMTypeOf(operand);
        if (!operandType || LLVMGetTypeKind(operandType) != LLVMIntegerTypeKind) {
            LLVMTypeRef intType = LLVMInt32TypeInContext(ctx->llvmContext);
            operand = cg_cast_value(ctx, operand, intType, stableParsed, NULL, "bnot.cast");
            if (!operand) {
                parsedTypeFree(&operandParsedCopy);
                fprintf(stderr, "Error: Failed to cast operand for bitwise not\n");
                return NULL;
            }
        }
        parsedTypeFree(&operandParsedCopy);
        return LLVMBuildNot(ctx->builder, operand, "nottmp");
    } else if (strcmp(node->expr.op, "!") == 0) {
        LLVMValueRef boolVal = cg_build_truthy(ctx, operand, stableParsed, "lnot.bool");
        parsedTypeFree(&operandParsedCopy);
        if (!boolVal) return NULL;
        LLVMTypeRef boolTy = LLVMTypeOf(boolVal);
        if (!boolTy || LLVMGetTypeKind(boolTy) != LLVMIntegerTypeKind) {
            fprintf(stderr, "Error: unary ! requires scalar operand\n");
            if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
            return NULL;
        }
        if (LLVMGetIntTypeWidth(boolTy) != 1) {
            LLVMValueRef zero = LLVMConstInt(boolTy, 0, 0);
            boolVal = LLVMBuildICmp(ctx->builder, LLVMIntNE, boolVal, zero, "lnot.bool.cast");
        }
        LLVMValueRef inverted = LLVMBuildNot(ctx->builder, boolVal, "lnot.tmp");
        return cg_widen_bool_to_int(ctx, inverted, "lnot.int");
    } else if (strcmp(node->expr.op, "*") == 0) {
        if (!cg_parsed_type_is_pointerish(stableParsed) &&
            node->expr.left &&
            node->expr.left->type == AST_UNARY_EXPRESSION &&
            node->expr.left->expr.op &&
            strcmp(node->expr.left->expr.op, "*") == 0) {
            const ParsedType* innerOperand = cg_resolve_expression_type(ctx, node->expr.left->expr.left);
            ParsedType innerOperandCopy = parsedTypeClone(innerOperand);
            derivedPointerParsed = parsedTypePointerTargetType(&innerOperandCopy);
            parsedTypeFree(&innerOperandCopy);
            if (derivedPointerParsed.kind != TYPE_INVALID) {
                stableParsed = &derivedPointerParsed;
                hasDerivedPointerParsed = true;
            }
        }
        size_t arrayDerivCount = stableParsed
            ? parsedTypeCountDerivationsOfKind(stableParsed, TYPE_DERIVATION_ARRAY)
            : 0;
        if (stableParsed &&
            !cg_parsed_type_is_pointerish(stableParsed) &&
            arrayDerivCount > 1) {
            parsedTypeFree(&operandParsedCopy);
            if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
            return operand;
        }
        LLVMTypeRef ptrType = LLVMTypeOf(operand);
        if (!ptrType || LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
            parsedTypeFree(&operandParsedCopy);
            if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
            fprintf(stderr, "Error: Cannot dereference non-pointer value\n");
            return NULL;
        }
        LLVMTypeRef elemType = NULL;
        ASTNode* baseExpr = node->expr.left;
        size_t derefCount = 1;
        while (baseExpr &&
               baseExpr->type == AST_UNARY_EXPRESSION &&
               baseExpr->expr.op &&
               strcmp(baseExpr->expr.op, "*") == 0) {
            derefCount++;
            baseExpr = baseExpr->expr.left;
        }
        if (baseExpr) {
            const ParsedType* baseType = NULL;
            if (baseExpr->type == AST_IDENTIFIER && ctx && ctx->currentScope) {
                NamedValue* entry = cg_scope_lookup(ctx->currentScope, baseExpr->valueNode.value);
                if (entry && entry->parsedType) {
                    baseType = entry->parsedType;
                }
            }
            if (!baseType) {
                baseType = cg_resolve_expression_type(ctx, baseExpr);
            }
            ParsedType chain = parsedTypeClone(baseType);
            bool ok = chain.kind != TYPE_INVALID;
            for (size_t i = 0; i < derefCount && ok; ++i) {
                ParsedType next = parsedTypePointerTargetType(&chain);
                parsedTypeFree(&chain);
                chain = next;
                ok = chain.kind != TYPE_INVALID;
            }
            if (ok) {
                elemType = cg_type_from_parsed(ctx, &chain);
            }
            parsedTypeFree(&chain);
        }
        if (!elemType) {
            elemType = cg_element_type_from_pointer(ctx, stableParsed, ptrType);
        }
        if (!elemType || LLVMGetTypeKind(elemType) == LLVMVoidTypeKind) {
            elemType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        parsedTypeFree(&operandParsedCopy);
        if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
        return LLVMBuildLoad2(ctx->builder, elemType, operand, "loadtmp");
    }

    parsedTypeFree(&operandParsedCopy);
    if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
    fprintf(stderr, "Error: Unsupported unary operator %s\n", node->expr.op);
    return NULL;
}

LLVMValueRef codegenAssignment(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_ASSIGNMENT) {
        fprintf(stderr, "Error: Invalid node type for codegenAssignment\n");
        return NULL;
    }

    CG_DEBUG("[CG] Assignment node\n");
    ASTNode* target = node->assignment.target;
    LLVMValueRef targetPtr = NULL;
    LLVMTypeRef targetType = NULL;
    const ParsedType* targetParsed = NULL;
    CGLValueInfo lvalInfo;
    LLVMValueRef srcPtr = NULL;
    LLVMTypeRef srcType = NULL;
    const ParsedType* srcParsed = NULL;
    if (!codegenLValue(ctx, target, &targetPtr, &targetType, &targetParsed, &lvalInfo)) {
        fprintf(stderr,
                "Error: Unsupported assignment target type %d at line %d\n",
                target ? target->type : -1,
                target ? target->line : -1);
        return NULL;
    }
    if (getenv("DEBUG_FLEX_ASSIGN")) {
        fprintf(stderr,
                "[DBG] assign flex flags: isFlexElement=%d size=%llu isBitfield=%d\n",
                lvalInfo.isFlexElement,
                (unsigned long long)lvalInfo.flexElemSize,
                lvalInfo.isBitfield);
    }

    LLVMValueRef value = NULL;
    const ParsedType* valueParsed = cg_resolve_expression_type(ctx, node->assignment.value);
    const char* op = node->assignment.op ? node->assignment.op : "=";

    bool destIsAggregate = targetType && (LLVMGetTypeKind(targetType) == LLVMStructTypeKind ||
                                          LLVMGetTypeKind(targetType) == LLVMArrayTypeKind);
    if (destIsAggregate) {
        bool isComplexTarget =
            cg_parsed_type_is_complex_value(targetParsed) || cg_llvm_type_is_complex_value(targetType);

        if (strcmp(op, "=") != 0) {
            value = codegenNode(ctx, node->assignment.value);
            if (!value) {
                fprintf(stderr, "Error: Assignment value failed to generate\n");
                return NULL;
            }
            if (!isComplexTarget ||
                (strcmp(op, "+=") != 0 && strcmp(op, "-=") != 0 &&
                 strcmp(op, "*=") != 0 && strcmp(op, "/=") != 0)) {
                fprintf(stderr, "Error: Unsupported compound assignment operator %s for aggregate target\n", op);
                return NULL;
            }

            LLVMValueRef current = cg_build_load(ctx, targetType, targetPtr, "complex.compound.load", targetParsed);
            if (!current) {
                fprintf(stderr, "Error: Failed to load complex target for compound assignment\n");
                return NULL;
            }

            LLVMTypeRef complexType = targetType;
            LLVMTypeRef elemType = cg_complex_element_type(ctx, targetParsed);
            LLVMTypeRef inferredElemType = cg_complex_element_type_from_llvm(complexType);
            if (inferredElemType) {
                elemType = inferredElemType;
            }
            LLVMValueRef lhsComplex = cg_promote_to_complex(ctx, current, targetParsed, complexType, elemType);
            LLVMValueRef rhsComplex = cg_promote_to_complex(ctx, value, valueParsed, complexType, elemType);
            if (!lhsComplex || !rhsComplex) {
                fprintf(stderr, "Error: Failed to promote complex operands for compound assignment\n");
                return NULL;
            }

            LLVMValueRef result = NULL;
            if (strcmp(op, "+=") == 0 || strcmp(op, "-=") == 0) {
                bool isSub = (strcmp(op, "-=") == 0);
                result = cg_build_complex_addsub(ctx, lhsComplex, rhsComplex, complexType, isSub);
            } else if (strcmp(op, "*=") == 0) {
                result = cg_build_complex_muldiv(ctx, lhsComplex, rhsComplex, complexType, false);
            } else if (strcmp(op, "/=") == 0) {
                result = cg_build_complex_muldiv(ctx, lhsComplex, rhsComplex, complexType, true);
            }
            if (!result) {
                fprintf(stderr, "Error: Failed to build complex compound assignment result\n");
                return NULL;
            }
            cg_build_store(ctx, result, targetPtr, targetParsed);
            return result;
        }

        if (!codegenLValue(ctx, node->assignment.value, &srcPtr, &srcType, &srcParsed, NULL)) {
            if (!value) {
                value = codegenNode(ctx, node->assignment.value);
                if (!value) {
                    fprintf(stderr, "Error: Assignment value failed to generate\n");
                    return NULL;
                }
            }
            LLVMValueRef tmp = cg_build_entry_alloca(ctx, targetType, "agg.tmp");
            LLVMBuildStore(ctx->builder, value, tmp);
            srcPtr = tmp;
            srcType = targetType;
            srcParsed = targetParsed;
        }
        uint64_t bytes = 0;
        uint32_t align = 0;
        cg_size_align_for_type(ctx, targetParsed, targetType, &bytes, &align);
        LLVMTypeRef i8Ptr = LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
        LLVMValueRef dstCast = LLVMBuildBitCast(ctx->builder, targetPtr, i8Ptr, "agg.dst");
        LLVMValueRef srcCast = LLVMBuildBitCast(ctx->builder, srcPtr, i8Ptr, "agg.src");
        LLVMValueRef sizeVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), bytes, 0);
        unsigned alignVal = align ? align : 1;
        LLVMBuildMemCpy(ctx->builder,
                        dstCast,
                        alignVal,
                        srcCast,
                        alignVal,
                        sizeVal);
        return value ? value : targetPtr;
    }

    value = codegenNode(ctx, node->assignment.value);
    if (!value) {
        fprintf(stderr, "Error: Assignment value failed to generate\n");
        return NULL;
    }

    LLVMTypeRef storeType = targetType;
    if (lvalInfo.isBitfield) {
        unsigned width = lvalInfo.layout.widthBits ? (unsigned)lvalInfo.layout.widthBits : LLVMGetIntTypeWidth(lvalInfo.storageType);
        if (width == 0) width = LLVMGetIntTypeWidth(lvalInfo.storageType);
        storeType = LLVMIntTypeInContext(ctx->llvmContext, width);
    }
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        storeType = cg_dereference_ptr_type(ctx, LLVMTypeOf(targetPtr), "assignment target");
    }
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        storeType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    if (lvalInfo.isFlexElement && lvalInfo.flexElemSize > 0) {
        unsigned bits = (unsigned)(lvalInfo.flexElemSize * 8);
        if (bits == 0) bits = 8;
        if (bits > 64) bits = 64;
        storeType = LLVMIntTypeInContext(ctx->llvmContext, bits);
    }
    LLVMValueRef storedValue = value;

    if (op && strcmp(op, "=") != 0) {
        LLVMValueRef current = NULL;
        if (lvalInfo.isBitfield) {
            current = cg_load_bitfield(ctx, &lvalInfo, storeType);
        } else {
            current = cg_build_load(ctx, storeType, targetPtr, "compound.load", targetParsed);
        }
        if (!current) {
            fprintf(stderr, "Error: Failed to load target for compound assignment\n");
            return NULL;
        }

        bool targetIsPointer = LLVMGetTypeKind(storeType) == LLVMPointerTypeKind;
        bool targetIsFloat = cg_is_float_type(LLVMTypeOf(current));
        if (targetIsPointer && (strcmp(op, "+=") == 0 || strcmp(op, "-=") == 0)) {
            bool isSubtract = (strcmp(op, "-=") == 0);
            storedValue = cg_build_pointer_offset(ctx, current, value, targetParsed, valueParsed, isSubtract);
            if (!storedValue) {
                return NULL;
            }
        } else if (strcmp(op, "+=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = targetIsFloat
                ? LLVMBuildFAdd(ctx->builder, current, value, "compound.fadd")
                : LLVMBuildAdd(ctx->builder, current, value, "compound.add");
        } else if (strcmp(op, "-=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = targetIsFloat
                ? LLVMBuildFSub(ctx->builder, current, value, "compound.fsub")
                : LLVMBuildSub(ctx->builder, current, value, "compound.sub");
        } else if (strcmp(op, "*=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = targetIsFloat
                ? LLVMBuildFMul(ctx->builder, current, value, "compound.fmul")
                : LLVMBuildMul(ctx->builder, current, value, "compound.mul");
        } else if (strcmp(op, "/=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            if (targetIsFloat) {
                storedValue = LLVMBuildFDiv(ctx->builder, current, value, "compound.fdiv");
            } else if (cg_expression_is_unsigned(ctx, node->assignment.target)) {
                storedValue = LLVMBuildUDiv(ctx->builder, current, value, "compound.div");
            } else {
                storedValue = LLVMBuildSDiv(ctx->builder, current, value, "compound.div");
            }
        } else if (strcmp(op, "%=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            if (targetIsFloat) {
                storedValue = LLVMBuildFRem(ctx->builder, current, value, "compound.frem");
            } else if (cg_expression_is_unsigned(ctx, node->assignment.target)) {
                storedValue = LLVMBuildURem(ctx->builder, current, value, "compound.rem");
            } else {
                storedValue = LLVMBuildSRem(ctx->builder, current, value, "compound.rem");
            }
        } else if (strcmp(op, "&=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildAnd(ctx->builder, current, value, "compound.and");
        } else if (strcmp(op, "|=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildOr(ctx->builder, current, value, "compound.or");
        } else if (strcmp(op, "^=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildXor(ctx->builder, current, value, "compound.xor");
        } else if (strcmp(op, "<<=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildShl(ctx->builder, current, value, "compound.shl");
        } else if (strcmp(op, ">>=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            if (cg_expression_is_unsigned(ctx, node->assignment.target)) {
                storedValue = LLVMBuildLShr(ctx->builder, current, value, "compound.lshr");
            } else {
                storedValue = LLVMBuildAShr(ctx->builder, current, value, "compound.ashr");
            }
        } else {
            fprintf(stderr, "Error: Unsupported compound assignment operator %s\n", op);
            return NULL;
        }
    }

    storedValue = cg_cast_value(ctx,
                                storedValue,
                                storeType,
                                NULL,
                                targetParsed,
                                "assign.cast");
    if (lvalInfo.isBitfield) {
        if (!cg_store_bitfield(ctx, &lvalInfo, storedValue)) {
            return NULL;
        }
        return storedValue;
    }
    if (lvalInfo.isFlexElement && lvalInfo.flexElemSize > 0) {
        if (getenv("DEBUG_FLEX_ASSIGN")) {
            char* tstr = storeType ? LLVMPrintTypeToString(storeType) : NULL;
            fprintf(stderr, "[DBG] flex store type=%s size=%llu\n", tstr ? tstr : "<null>",
                    (unsigned long long)lvalInfo.flexElemSize);
            if (tstr) LLVMDisposeMessage(tstr);
        }
        LLVMTypeRef i8Ptr = LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
        LLVMValueRef dstCast = LLVMBuildBitCast(ctx->builder, targetPtr, i8Ptr, "flex.dst");
        LLVMValueRef srcCast = NULL;
        ASTNode* rhs = node->assignment.value;
        bool rhsIsLValue = rhs &&
                           (rhs->type == AST_IDENTIFIER || rhs->type == AST_ARRAY_ACCESS ||
                            rhs->type == AST_POINTER_ACCESS || rhs->type == AST_DOT_ACCESS ||
                            rhs->type == AST_COMPOUND_LITERAL);
        if (rhsIsLValue &&
            codegenLValue(ctx, rhs, &srcPtr, &srcType, &srcParsed, NULL)) {
            srcCast = LLVMBuildBitCast(ctx->builder, srcPtr, i8Ptr, "flex.src");
        } else {
            LLVMValueRef tmp = cg_build_entry_alloca(ctx, storeType, "flex.tmp");
            LLVMBuildStore(ctx->builder, storedValue, tmp);
            srcCast = LLVMBuildBitCast(ctx->builder, tmp, i8Ptr, "flex.src.tmp");
        }
        LLVMValueRef sizeVal =
            LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), lvalInfo.flexElemSize, 0);
        LLVMBuildMemCpy(ctx->builder, dstCast, 1, srcCast, 1, sizeVal);
        return storedValue;
    }
    cg_build_store(ctx, storedValue, targetPtr, targetParsed);
    return storedValue;
}
