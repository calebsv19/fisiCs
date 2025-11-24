#include "codegen_private.h"

#include "codegen_types.h"

#include <stdio.h>
#include <string.h>

LLVMValueRef codegenBinaryExpression(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_BINARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenBinaryExpression\n");
        return NULL;
    }

    const char* op = node->expr.op ? node->expr.op : "";
    LLVMValueRef L = codegenNode(ctx, node->expr.left);
    if (!L) {
        fprintf(stderr, "Error: Failed to generate LHS for binary expression\n");
        return NULL;
    }

    if (strcmp(op, "&&") == 0) {
        return codegenLogicalAndOr(ctx, L, node->expr.right, true);
    }
    if (strcmp(op, "||") == 0) {
        return codegenLogicalAndOr(ctx, L, node->expr.right, false);
    }

    LLVMValueRef R = codegenNode(ctx, node->expr.right);
    if (!R) {
        fprintf(stderr, "Error: Failed to generate RHS for binary expression\n");
        return NULL;
    }

    const ParsedType* lhsParsed = cg_resolve_expression_type(ctx, node->expr.left);
    const ParsedType* rhsParsed = cg_resolve_expression_type(ctx, node->expr.right);
    LLVMTypeRef lhsType = LLVMTypeOf(L);
    LLVMTypeRef rhsType = LLVMTypeOf(R);
    bool lhsIsPointer = LLVMGetTypeKind(lhsType) == LLVMPointerTypeKind;
    bool rhsIsPointer = LLVMGetTypeKind(rhsType) == LLVMPointerTypeKind;
    bool preferUnsigned = cg_expression_is_unsigned(ctx, node->expr.left) ||
                          cg_expression_is_unsigned(ctx, node->expr.right);

    if (strcmp(op, "+") == 0) {
        if (lhsIsPointer && rhsIsPointer) {
            fprintf(stderr, "Error: cannot add two pointer values\n");
            return NULL;
        }
        if (lhsIsPointer) {
            return cg_build_pointer_offset(ctx, L, R, lhsParsed, rhsParsed, false);
        }
        if (rhsIsPointer) {
            return cg_build_pointer_offset(ctx, R, L, rhsParsed, lhsParsed, false);
        }
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "add.cast");
        }
        return LLVMBuildAdd(ctx->builder, L, R, "addtmp");
    } else if (strcmp(op, "-") == 0) {
        if (lhsIsPointer && rhsIsPointer) {
            return cg_build_pointer_difference(ctx, L, R, lhsParsed ? lhsParsed : rhsParsed);
        }
        if (lhsIsPointer) {
            return cg_build_pointer_offset(ctx, L, R, lhsParsed, rhsParsed, true);
        }
        if (rhsIsPointer) {
            fprintf(stderr, "Error: cannot subtract pointer from integer\n");
            return NULL;
        }
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "sub.cast");
        }
        return LLVMBuildSub(ctx->builder, L, R, "subtmp");
    } else if (strcmp(op, "*") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "mul.cast");
        }
        return LLVMBuildMul(ctx->builder, L, R, "multmp");
    } else if (strcmp(op, "/") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "div.cast");
        }
        if (preferUnsigned) {
            return LLVMBuildUDiv(ctx->builder, L, R, "divtmp");
        }
        return LLVMBuildSDiv(ctx->builder, L, R, "divtmp");
    } else if (strcmp(op, "%") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "rem.cast");
        }
        if (preferUnsigned) {
            return LLVMBuildURem(ctx->builder, L, R, "modtmp");
        }
        return LLVMBuildSRem(ctx->builder, L, R, "modtmp");
    } else if (strcmp(op, "==") == 0 ||
               strcmp(op, "!=") == 0 ||
               strcmp(op, "<") == 0 ||
               strcmp(op, "<=") == 0 ||
               strcmp(op, ">") == 0 ||
               strcmp(op, ">=") == 0) {
        if (lhsIsPointer || rhsIsPointer) {
            if (lhsIsPointer && !rhsIsPointer) {
                LLVMTypeRef ptrType = LLVMTypeOf(L);
                LLVMValueRef casted = cg_cast_value(ctx, R, cg_get_intptr_type(ctx), rhsParsed, NULL, "ptr.cmp.int");
                R = LLVMBuildIntToPtr(ctx->builder, casted, ptrType, "ptr.cmp.inttoptr");
                rhsIsPointer = true;
            } else if (!lhsIsPointer && rhsIsPointer) {
                LLVMTypeRef ptrType = LLVMTypeOf(R);
                LLVMValueRef casted = cg_cast_value(ctx, L, cg_get_intptr_type(ctx), lhsParsed, NULL, "ptr.cmp.int");
                L = LLVMBuildIntToPtr(ctx->builder, casted, ptrType, "ptr.cmp.inttoptr");
                lhsIsPointer = true;
            }
            if (!lhsIsPointer || !rhsIsPointer) {
                fprintf(stderr, "Error: pointer comparison requires pointer-compatible operands\n");
                return NULL;
            }
            LLVMTypeRef ptrTy = LLVMTypeOf(L);
            if (ptrTy != LLVMTypeOf(R)) {
                R = LLVMBuildBitCast(ctx->builder, R, ptrTy, "ptr.cmp.cast");
            }
            LLVMValueRef lhsInt = LLVMBuildPtrToInt(ctx->builder, L, cg_get_intptr_type(ctx), "ptr.cmp.lhs");
            LLVMValueRef rhsInt = LLVMBuildPtrToInt(ctx->builder, R, cg_get_intptr_type(ctx), "ptr.cmp.rhs");
            LLVMIntPredicate pred = cg_select_int_predicate(op, true);
            LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, pred, lhsInt, rhsInt, "ptrcmp");
            return cg_widen_bool_to_int(ctx, cmp, "ptrcmp.int");
        }
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "cmp.cast");
        }
        LLVMIntPredicate pred = cg_select_int_predicate(op, preferUnsigned);
        LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, pred, L, R, "cmptmp");
        return cg_widen_bool_to_int(ctx, cmp, "cmp.int");
    } else if (strcmp(op, "&") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "and.cast");
        }
        return LLVMBuildAnd(ctx->builder, L, R, "andtmp");
    } else if (strcmp(op, "|") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "or.cast");
        }
        return LLVMBuildOr(ctx->builder, L, R, "ortmp");
    } else if (strcmp(op, "^") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "xor.cast");
        }
        return LLVMBuildXor(ctx->builder, L, R, "xortmp");
    } else if (strcmp(op, "<<") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "shl.cast");
        }
        return LLVMBuildShl(ctx->builder, L, R, "shltmp");
    } else if (strcmp(op, ">>") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "shr.cast");
        }
        if (preferUnsigned) {
            return LLVMBuildLShr(ctx->builder, L, R, "shrtmp");
        }
        return LLVMBuildAShr(ctx->builder, L, R, "shrtmp");
    }

    fprintf(stderr, "Error: Unsupported binary operator %s\n", op);
    return NULL;
}


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
        if (!codegenLValue(ctx, node->expr.left, &targetPtr, &targetType, &targetParsed)) {
            fprintf(stderr, "Error: ++/-- operand must be assignable\n");
            return NULL;
        }

        LLVMValueRef current = LLVMBuildLoad2(ctx->builder, targetType, targetPtr, "unary.load");
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
            LLVMBuildStore(ctx->builder, updated, targetPtr);
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

        LLVMBuildStore(ctx->builder, updated, targetPtr);
        return isPostfix ? current : updated;
    }

    const ParsedType* operandParsed = cg_resolve_expression_type(ctx, node->expr.left);
    LLVMValueRef operand = codegenNode(ctx, node->expr.left);
    if (!operand) {
        fprintf(stderr, "Error: Failed to generate operand for unary expression\n");
        return NULL;
    }

    if (strcmp(node->expr.op, "-") == 0) {
        return LLVMBuildNeg(ctx->builder, operand, "negtmp");
    } else if (strcmp(node->expr.op, "!") == 0) {
        LLVMValueRef boolVal = cg_build_truthy(ctx, operand, operandParsed, "lnot.bool");
        if (!boolVal) return NULL;
        LLVMValueRef inverted = LLVMBuildNot(ctx->builder, boolVal, "lnot.tmp");
        return cg_widen_bool_to_int(ctx, inverted, "lnot.int");
    } else if (strcmp(node->expr.op, "&") == 0) {
        LLVMValueRef addrPtr = NULL;
        LLVMTypeRef addrType = NULL;
        if (!codegenLValue(ctx, node->expr.left, &addrPtr, &addrType, NULL)) {
            fprintf(stderr, "Error: Address-of requires an lvalue\n");
            return NULL;
        }
        return addrPtr;
    } else if (strcmp(node->expr.op, "*") == 0) {
        LLVMTypeRef ptrType = LLVMTypeOf(operand);
        if (!ptrType || LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
            fprintf(stderr, "Error: Cannot dereference non-pointer value\n");
            return NULL;
        }
        LLVMTypeRef elemType = cg_element_type_from_pointer(ctx, operandParsed, ptrType);
        if (!elemType || LLVMGetTypeKind(elemType) == LLVMVoidTypeKind) {
            elemType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        return LLVMBuildLoad2(ctx->builder, elemType, operand, "loadtmp");
    }

    fprintf(stderr, "Error: Unsupported unary operator %s\n", node->expr.op);
    return NULL;
}


LLVMValueRef codegenTernaryExpression(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_TERNARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenTernaryExpression\n");
        return NULL;
    }

    LLVMValueRef condition = codegenNode(ctx, node->ternaryExpr.condition);
    if (!condition) {
        fprintf(stderr, "Error: Failed to generate condition for ternary expression\n");
        return NULL;
    }

    condition = cg_build_truthy(ctx, condition, NULL, "ternaryCond");
    if (!condition) {
        fprintf(stderr, "Error: Failed to convert ternary condition to boolean\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));

    LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(func, "ternaryTrue");
    LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(func, "ternaryFalse");
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(func, "ternaryMerge");

    LLVMBuildCondBr(ctx->builder, condition, trueBB, falseBB);

    LLVMPositionBuilderAtEnd(ctx->builder, trueBB);
    LLVMValueRef trueValue = codegenNode(ctx, node->ternaryExpr.trueExpr);
    LLVMBuildBr(ctx->builder, mergeBB);

    LLVMPositionBuilderAtEnd(ctx->builder, falseBB);
    LLVMValueRef falseValue = codegenNode(ctx, node->ternaryExpr.falseExpr);
    LLVMBuildBr(ctx->builder, mergeBB);

    LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
    LLVMValueRef phi = LLVMBuildPhi(ctx->builder, LLVMInt32TypeInContext(ctx->llvmContext), "ternaryResult");
    LLVMAddIncoming(phi, &trueValue, &trueBB, 1);
    LLVMAddIncoming(phi, &falseValue, &falseBB, 1);

    return phi;
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
    if (!codegenLValue(ctx, target, &targetPtr, &targetType, &targetParsed)) {
        fprintf(stderr, "Error: Unsupported assignment target type %d\n", target ? target->type : -1);
        return NULL;
    }

    LLVMValueRef value = codegenNode(ctx, node->assignment.value);
    if (!value) {
        fprintf(stderr, "Error: Assignment value failed to generate\n");
        return NULL;
    }
    const ParsedType* valueParsed = cg_resolve_expression_type(ctx, node->assignment.value);

    const char* op = node->assignment.op ? node->assignment.op : "=";
    LLVMTypeRef storeType = targetType;
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        storeType = LLVMGetElementType(LLVMTypeOf(targetPtr));
    }
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        storeType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    LLVMValueRef storedValue = value;

    if (op && strcmp(op, "=") != 0) {
        LLVMValueRef current = LLVMBuildLoad2(ctx->builder, storeType, targetPtr, "compound.load");
        if (!current) {
            fprintf(stderr, "Error: Failed to load target for compound assignment\n");
            return NULL;
        }

        bool targetIsPointer = LLVMGetTypeKind(storeType) == LLVMPointerTypeKind;
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
            storedValue = LLVMBuildAdd(ctx->builder, current, value, "compound.add");
        } else if (strcmp(op, "-=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildSub(ctx->builder, current, value, "compound.sub");
        } else if (strcmp(op, "*=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildMul(ctx->builder, current, value, "compound.mul");
        } else if (strcmp(op, "/=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            if (cg_expression_is_unsigned(ctx, node->assignment.target)) {
                storedValue = LLVMBuildUDiv(ctx->builder, current, value, "compound.div");
            } else {
                storedValue = LLVMBuildSDiv(ctx->builder, current, value, "compound.div");
            }
        } else if (strcmp(op, "%=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            if (cg_expression_is_unsigned(ctx, node->assignment.target)) {
                storedValue = LLVMBuildURem(ctx->builder, current, value, "compound.rem");
            } else {
                storedValue = LLVMBuildSRem(ctx->builder, current, value, "compound.rem");
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
    return LLVMBuildStore(ctx->builder, storedValue, targetPtr);
}


LLVMValueRef codegenArrayAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_ARRAY_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenArrayAccess\n");
        return NULL;
    }

    LLVMValueRef arrayPtr = codegenNode(ctx, node->arrayAccess.array);
    LLVMValueRef index = codegenNode(ctx, node->arrayAccess.index);
    if (!arrayPtr || !index) {
        fprintf(stderr, "Error: Array access failed\n");
        return NULL;
    }
    char* arrTyStr = LLVMPrintTypeToString(LLVMTypeOf(arrayPtr));
    CG_DEBUG("[CG] Array access base type: %s\n", arrTyStr ? arrTyStr : "<null>");
    if (arrTyStr) LLVMDisposeMessage(arrTyStr);
    LLVMTypeRef aggregateHint = NULL;
    LLVMTypeRef elementHint = NULL;
    if (node->arrayAccess.array && node->arrayAccess.array->type == AST_IDENTIFIER) {
        NamedValue* entry = cg_scope_lookup(ctx->currentScope, node->arrayAccess.array->valueNode.value);
        if (entry) {
            aggregateHint = entry->type;
            char* hintStr = aggregateHint ? LLVMPrintTypeToString(aggregateHint) : NULL;
            CG_DEBUG("[CG] Array access aggregate hint=%s\n", hintStr ? hintStr : "<null>");
            if (hintStr) LLVMDisposeMessage(hintStr);
            elementHint = entry->elementType;
        }
    }
    LLVMTypeRef derivedElementType = NULL;
    LLVMValueRef elementPtr = buildArrayElementPointer(ctx, arrayPtr, index, aggregateHint, elementHint, &derivedElementType);
    if (!elementPtr) {
        fprintf(stderr, "Error: Failed to compute array element pointer\n");
        return NULL;
    }
    CG_DEBUG("[CG] Array element pointer computed\n");
    LLVMTypeRef ptrToElemType = LLVMTypeOf(elementPtr);
    char* ptrElemStr = ptrToElemType ? LLVMPrintTypeToString(ptrToElemType) : NULL;
    CG_DEBUG("[CG] Array element pointer LLVM type=%s\n", ptrElemStr ? ptrElemStr : "<null>");
    if (ptrElemStr) LLVMDisposeMessage(ptrElemStr);

    LLVMTypeRef elementType = derivedElementType;
    if (!elementType) {
        elementType = LLVMGetElementType(ptrToElemType);
        if (!elementType || LLVMGetTypeKind(elementType) == LLVMVoidTypeKind) {
            elementType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
    }
    LLVMValueRef typedElementPtr = elementPtr;
    LLVMTypeRef expectedPtrType = LLVMPointerType(elementType, 0);
    if (LLVMTypeOf(elementPtr) != expectedPtrType) {
        typedElementPtr = LLVMBuildBitCast(ctx->builder, elementPtr, expectedPtrType, "array.elem.cast");
    }
    LLVMValueRef loadVal = LLVMBuildLoad2(ctx->builder, elementType, typedElementPtr, "arrayLoad");
    CG_DEBUG("[CG] Array element load complete\n");
    return loadVal;
}


LLVMValueRef codegenPointerAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_POINTER_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenPointerAccess\n");
        return NULL;
    }

    LLVMValueRef basePtr = codegenNode(ctx, node->memberAccess.base);
    if (!basePtr) return NULL;
    LLVMTypeRef basePtrType = LLVMTypeOf(basePtr);
    LLVMTypeRef aggregateType = NULL;
    const char* nameHint = NULL;
    if (LLVMGetTypeKind(basePtrType) == LLVMPointerTypeKind) {
        aggregateType = LLVMGetElementType(basePtrType);
        if (aggregateType && LLVMGetTypeKind(aggregateType) == LLVMStructTypeKind) {
            nameHint = LLVMGetStructName(aggregateType);
        }
    }
    LLVMTypeRef fieldType = NULL;
    LLVMValueRef fieldPtr = buildStructFieldPointer(ctx,
                                                    basePtr,
                                                    aggregateType,
                                                    nameHint,
                                                    node->memberAccess.field,
                                                    &fieldType,
                                                    NULL);
    if (!fieldPtr) {
        fprintf(stderr, "Error: Unknown field '%s'\n", node->memberAccess.field);
        return NULL;
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

    CG_DEBUG("[CG] Dot access begin\n");
    LLVMValueRef basePtr = NULL;
    LLVMTypeRef baseType = NULL;
    if (!codegenLValue(ctx, node->memberAccess.base, &basePtr, &baseType, NULL)) {
        CG_DEBUG("[CG] Dot access fallback to value\n");
        LLVMValueRef baseVal = codegenNode(ctx, node->memberAccess.base);
        if (!baseVal) return NULL;
        if (LLVMGetTypeKind(LLVMTypeOf(baseVal)) == LLVMPointerTypeKind) {
            basePtr = baseVal;
            baseType = LLVMGetElementType(LLVMTypeOf(basePtr));
        } else {
            LLVMValueRef tmpAlloca = LLVMBuildAlloca(ctx->builder, LLVMTypeOf(baseVal), "dot_tmp_ptr");
            LLVMBuildStore(ctx->builder, baseVal, tmpAlloca);
            basePtr = tmpAlloca;
            baseType = LLVMTypeOf(baseVal);
        }
    }

    const char* nameHint = NULL;
    if (baseType && LLVMGetTypeKind(baseType) == LLVMStructTypeKind) {
        nameHint = LLVMGetStructName(baseType);
    }

    LLVMTypeRef fieldType = NULL;
    LLVMValueRef fieldPtr = buildStructFieldPointer(ctx,
                                                    basePtr,
                                                    baseType,
                                                    nameHint,
                                                    node->memberAccess.field,
                                                    &fieldType,
                                                    NULL);
    if (!fieldPtr) {
        char* tyStr = LLVMPrintTypeToString(LLVMTypeOf(basePtr));
        CG_TRACE("DEBUG: dot access base type %s\n", tyStr ? tyStr : "<null>");
        if (tyStr) { LLVMDisposeMessage(tyStr); }
        fprintf(stderr, "Error: Unknown field '%s'\n", node->memberAccess.field);
        return NULL;
    }
    char* ptrTyStr = LLVMPrintTypeToString(LLVMTypeOf(fieldPtr));
    CG_DEBUG("[CG] Dot access got field pointer type=%s\n", ptrTyStr ? ptrTyStr : "<null>");
    if (ptrTyStr) LLVMDisposeMessage(ptrTyStr);
    if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
        fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    char* fieldTyStr = LLVMPrintTypeToString(fieldType);
    CG_DEBUG("[CG] Dot access loading type=%s\n", fieldTyStr ? fieldTyStr : "<null>");
    if (fieldTyStr) LLVMDisposeMessage(fieldTyStr);
    LLVMValueRef result = LLVMBuildLoad2(ctx->builder, fieldType, fieldPtr, "dot_field");
    CG_DEBUG("[CG] Dot access load complete\n");
    return result;
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
    LLVMTypeRef elemType = LLVMGetElementType(LLVMTypeOf(pointer));
    if (!elemType || LLVMGetTypeKind(elemType) == LLVMVoidTypeKind) {
        elemType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return LLVMBuildLoad2(ctx->builder, elemType, pointer, "ptrLoad");
}


LLVMValueRef codegenLogicalAndOr(CodegenContext* ctx,
                                        LLVMValueRef lhsValue,
                                        ASTNode* rhsNode,
                                        bool isAnd) {
    if (!ctx || !rhsNode) {
        return NULL;
    }

    LLVMValueRef lhsBool = cg_build_truthy(ctx, lhsValue, NULL, isAnd ? "land.lhs" : "lor.lhs");
    if (!lhsBool) {
        fprintf(stderr, "Error: Failed to convert logical operand to boolean\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef lhsBlock = LLVMGetInsertBlock(ctx->builder);
    LLVMBasicBlockRef rhsBB = LLVMAppendBasicBlock(func, isAnd ? "land.rhs" : "lor.rhs");
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(func, isAnd ? "land.end" : "lor.end");

    if (isAnd) {
        LLVMBuildCondBr(ctx->builder, lhsBool, rhsBB, mergeBB);
    } else {
        LLVMBuildCondBr(ctx->builder, lhsBool, mergeBB, rhsBB);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, rhsBB);
    LLVMValueRef rhsValue = codegenNode(ctx, rhsNode);
    if (!rhsValue) {
        fprintf(stderr, "Error: Failed to generate logical RHS\n");
        return NULL;
    }
    LLVMValueRef rhsBool = cg_build_truthy(ctx, rhsValue, NULL, isAnd ? "land.rhs.bool" : "lor.rhs.bool");
    if (!rhsBool) {
        fprintf(stderr, "Error: Failed to convert logical RHS to boolean\n");
        return NULL;
    }
    LLVMBasicBlockRef rhsEvalBB = LLVMGetInsertBlock(ctx->builder);
    LLVMBuildBr(ctx->builder, mergeBB);

    LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
    LLVMValueRef phi = LLVMBuildPhi(ctx->builder, LLVMInt1TypeInContext(ctx->llvmContext),
                                    isAnd ? "land.result" : "lor.result");
    LLVMValueRef incomingVals[2];
    LLVMBasicBlockRef incomingBlocks[2];

    if (isAnd) {
        incomingVals[0] = LLVMConstInt(LLVMInt1TypeInContext(ctx->llvmContext), 0, 0);
        incomingBlocks[0] = lhsBlock;
        incomingVals[1] = rhsBool;
        incomingBlocks[1] = rhsEvalBB;
    } else {
        incomingVals[0] = LLVMConstInt(LLVMInt1TypeInContext(ctx->llvmContext), 1, 0);
        incomingBlocks[0] = lhsBlock;
        incomingVals[1] = rhsBool;
        incomingBlocks[1] = rhsEvalBB;
    }
    LLVMAddIncoming(phi, incomingVals, incomingBlocks, 2);
    return cg_widen_bool_to_int(ctx, phi, isAnd ? "land.int" : "lor.int");
}


LLVMValueRef codegenFunctionCall(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_FUNCTION_CALL) {
        fprintf(stderr, "Error: Invalid node type for codegenFunctionCall\n");
        return NULL;
    }

    if (!node->functionCall.callee || node->functionCall.callee->type != AST_IDENTIFIER) {
        fprintf(stderr, "Error: Unsupported callee in function call\n");
        return NULL;
    }

    LLVMValueRef function = LLVMGetNamedFunction(ctx->module,
                                                 node->functionCall.callee->valueNode.value);
    if (!function) {
        fprintf(stderr, "Error: Undefined function %s\n",
                node->functionCall.callee->valueNode.value);
        return NULL;
    }

    char* rawFnType = LLVMPrintTypeToString(LLVMTypeOf(function));
    CG_DEBUG("[CG] Function call raw type: %s\n", rawFnType ? rawFnType : "<null>");
    if (rawFnType) LLVMDisposeMessage(rawFnType);

    LLVMValueRef* args = NULL;
    if (node->functionCall.argumentCount > 0) {
        args = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * node->functionCall.argumentCount);
        if (!args) return NULL;
        for (size_t i = 0; i < node->functionCall.argumentCount; i++) {
            args[i] = codegenNode(ctx, node->functionCall.arguments[i]);
        }
    }

    LLVMTypeRef calleeType = NULL;
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (model) {
        const char* name = node->functionCall.callee->valueNode.value;
        const Symbol* sym = semanticModelLookupGlobal(model, name);
        if (sym && sym->kind == SYMBOL_FUNCTION) {
            LLVMTypeRef returnType = cg_type_from_parsed(ctx, &sym->type);
            if (!returnType || LLVMGetTypeKind(returnType) == LLVMVoidTypeKind) {
                returnType = LLVMVoidTypeInContext(ctx->llvmContext);
            }

            size_t paramCount = 0;
            ASTNode** params = NULL;
            if (sym->definition) {
                if (sym->definition->type == AST_FUNCTION_DEFINITION) {
                    paramCount = sym->definition->functionDef.paramCount;
                    params = sym->definition->functionDef.parameters;
                } else if (sym->definition->type == AST_FUNCTION_DECLARATION) {
                    paramCount = sym->definition->functionDecl.paramCount;
                    params = sym->definition->functionDecl.parameters;
                }
            }

            LLVMTypeRef* paramTypes = collectParamTypes(ctx, paramCount, params);
            calleeType = LLVMFunctionType(returnType, paramTypes, (unsigned)paramCount, 0);
            free(paramTypes);
        }
    }

    if (!calleeType) {
        calleeType = LLVMTypeOf(function);
        if (LLVMGetTypeKind(calleeType) == LLVMPointerTypeKind) {
            LLVMTypeRef element = LLVMGetElementType(calleeType);
            if (element) {
                calleeType = element;
            }
        }
    }

    char* resolvedFnType = LLVMPrintTypeToString(calleeType);
    CG_DEBUG("[CG] Function call resolved type: %s\n", resolvedFnType ? resolvedFnType : "<null>");
    if (resolvedFnType) LLVMDisposeMessage(resolvedFnType);

    LLVMValueRef call = LLVMBuildCall2(ctx->builder,
                                       calleeType,
                                       function,
                                       args,
                                       node->functionCall.argumentCount,
                                       "calltmp");
    free(args);
    return call;
}


LLVMValueRef codegenCastExpression(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_CAST_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenCastExpression\n");
        return NULL;
    }

    LLVMValueRef value = codegenNode(ctx, node->castExpr.expression);
    if (!value) {
        return NULL;
    }

    LLVMTypeRef target = cg_type_from_parsed(ctx, &node->castExpr.castType);
    if (!target || LLVMGetTypeKind(target) == LLVMVoidTypeKind) {
        return value; /* no conversion performed yet */
    }

    LLVMTypeRef valueType = LLVMTypeOf(value);
    if (valueType == target) {
        return value;
    }

    if (LLVMGetTypeKind(target) == LLVMPointerTypeKind && LLVMGetTypeKind(valueType) == LLVMPointerTypeKind) {
        return LLVMBuildBitCast(ctx->builder, value, target, "ptrcast");
    }

    if (LLVMGetTypeKind(target) == LLVMIntegerTypeKind && LLVMGetTypeKind(valueType) == LLVMIntegerTypeKind) {
        unsigned valueBits = LLVMGetIntTypeWidth(valueType);
        unsigned targetBits = LLVMGetIntTypeWidth(target);
        if (valueBits == targetBits) {
            return LLVMBuildBitCast(ctx->builder, value, target, "intcast");
        } else if (valueBits < targetBits) {
            return LLVMBuildSExt(ctx->builder, value, target, "extcast");
        } else {
            return LLVMBuildTrunc(ctx->builder, value, target, "truncast");
        }
    }

    /* Fallback: rely on bitcast even if the types differ (TODO: semantic-driven casts). */
    return LLVMBuildBitCast(ctx->builder, value, target, "casttmp");
}


LLVMValueRef codegenCompoundLiteral(CodegenContext* ctx, ASTNode* node) {
    if (!node || node->type != AST_COMPOUND_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenCompoundLiteral\n");
        return NULL;
    }

    LLVMTypeRef literalType = cg_type_from_parsed(ctx, &node->compoundLiteral.literalType);
    if (!literalType || LLVMGetTypeKind(literalType) == LLVMVoidTypeKind) {
        fprintf(stderr, "Warning: compound literal missing explicit type; defaulting to i32\n");
        literalType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    LLVMValueRef tmp = LLVMBuildAlloca(ctx->builder, literalType, "compound.tmp");
    if (!cg_store_compound_literal_into_ptr(ctx,
                                            tmp,
                                            literalType,
                                            &node->compoundLiteral.literalType,
                                            node)) {
        fprintf(stderr, "Error: Failed to emit compound literal\n");
        return LLVMConstNull(literalType);
    }

    return LLVMBuildLoad2(ctx->builder, literalType, tmp, "compound.value");
}


LLVMValueRef codegenNumberLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_NUMBER_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenNumberLiteral\n");
        return NULL;
    }

    int value = atoi(node->valueNode.value);
    return LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), value, 0);
}


LLVMValueRef codegenCharLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_CHAR_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenCharLiteral\n");
        return NULL;
    }

    char value = node->valueNode.value[0];
    return LLVMConstInt(LLVMInt8TypeInContext(ctx->llvmContext), value, 0);
}


LLVMValueRef codegenStringLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_STRING_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenStringLiteral\n");
        return NULL;
    }

    return LLVMBuildGlobalStringPtr(ctx->builder, node->valueNode.value, "str");
}


LLVMValueRef codegenIdentifier(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_IDENTIFIER) {
        fprintf(stderr, "Error: Invalid node type for codegenIdentifier\n");
        return NULL;
    }

    NamedValue* entry = cg_scope_lookup(ctx->currentScope, node->valueNode.value);
    if (entry) {
        if (entry->addressOnly) {
            return entry->value;
        }

        LLVMTypeRef loadType = entry->type;
        if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
            loadType = LLVMGetElementType(LLVMTypeOf(entry->value));
        }
        if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
            loadType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        LLVMValueRef loaded = LLVMBuildLoad2(ctx->builder, loadType, entry->value, node->valueNode.value);
        return loaded;
    }

    LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, node->valueNode.value);
    if (!global) {
        fprintf(stderr, "Error: Undefined variable %s\n", node->valueNode.value);
        return NULL;
    }

    LLVMTypeRef loadType = LLVMGetElementType(LLVMTypeOf(global));
    if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
        loadType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    const ParsedType* parsedType = NULL;
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
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
    return LLVMBuildLoad2(ctx->builder, loadType, global, node->valueNode.value);
}


LLVMValueRef codegenSizeof(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_SIZEOF) {
        fprintf(stderr, "Error: Invalid node type for codegenSizeof\n");
        return NULL;
    }

    LLVMTypeRef type = LLVMInt32TypeInContext(ctx->llvmContext);
    if (node->expr.left) {
        ASTNode* operand = node->expr.left;
        if (operand->type == AST_PARSED_TYPE) {
            type = cg_type_from_parsed(ctx, &operand->parsedTypeNode.parsed);
        } else {
            LLVMValueRef value = codegenNode(ctx, operand);
            if (value) {
                type = LLVMTypeOf(value);
            }
        }
    }

    return LLVMSizeOf(type);
}


LLVMValueRef codegenHeapAllocation(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_HEAP_ALLOCATION) {
        fprintf(stderr, "Error: Invalid node type for codegenHeapAllocation\n");
        return NULL;
    }

    LLVMTypeRef allocType = LLVMStructType(NULL, 0, 0);
    LLVMValueRef size = LLVMSizeOf(allocType);
    LLVMValueRef mallocFunc = LLVMGetNamedFunction(ctx->module, "malloc");

    if (!mallocFunc) {
        fprintf(stderr, "Error: malloc function not found\n");
        return NULL;
    }

    LLVMValueRef args[] = {size};
    return LLVMBuildCall2(ctx->builder, LLVMTypeOf(mallocFunc), mallocFunc, args, 1, "mallocCall");
}
