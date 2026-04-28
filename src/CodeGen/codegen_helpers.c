// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include "codegen_types.h"
#include "Syntax/literal_utils.h"
#include "Syntax/Expr/analyze_expr_internal.h"

#include <llvm-c/Core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void cg_clear_function_pointer_metadata(ParsedType* type) {
    if (!type) return;
    if (type->fpParams) {
        for (size_t i = 0; i < type->fpParamCount; ++i) {
            parsedTypeFree(&type->fpParams[i]);
        }
        free(type->fpParams);
        type->fpParams = NULL;
    }
    type->fpParamCount = 0;
    type->isFunctionPointer = false;
}

static bool cg_parsed_type_has_pointer_layer(const ParsedType* type) {
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
static const ParsedType* cg_builtin_signed_int_type(void) {
    static ParsedType type = {
        .kind = TYPE_PRIMITIVE,
        .primitiveType = TOKEN_INT,
        .pointerDepth = 0,
        .isUnsigned = false,
    };
    return &type;
}
static const ParsedType* cg_builtin_unsigned_int_type(void) {
    static ParsedType type = {
        .kind = TYPE_PRIMITIVE,
        .primitiveType = TOKEN_UNSIGNED,
        .pointerDepth = 0,
        .isUnsigned = true,
    };
    return &type;
}
static bool cg_literal_looks_float(const char* text) {
    if (!text) return false;
    bool isHex = text[0] == '0' && (text[1] == 'x' || text[1] == 'X');
    bool hasFloatMarker = false;
    for (const char* p = text; *p; ++p) {
        if (*p == '.') {
            hasFloatMarker = true;
        }
        if (!isHex && (*p == 'e' || *p == 'E')) {
            hasFloatMarker = true;
        }
        if (isHex && (*p == 'p' || *p == 'P')) {
            hasFloatMarker = true;
        }
    }
    if (hasFloatMarker) {
        return true;
    }
    size_t len = strlen(text);
    if (len > 0) {
        char last = text[len - 1];
        if (!isHex && (last == 'f' || last == 'F')) {
            return true;
        }
    }
    return false;
}
static ParsedType cg_make_float_literal_type(const char* text) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_PRIMITIVE;
    t.primitiveType = TOKEN_DOUBLE;
    size_t len = text ? strlen(text) : 0;
    bool hasImag = len > 0 &&
                   (text[len - 1] == 'i' || text[len - 1] == 'I' ||
                    text[len - 1] == 'j' || text[len - 1] == 'J');
    size_t coreLen = hasImag && len > 0 ? len - 1 : len;
    bool isFloat = coreLen > 0 &&
                   (text[coreLen - 1] == 'f' || text[coreLen - 1] == 'F');
    bool isLong = coreLen > 0 &&
                  (text[coreLen - 1] == 'l' || text[coreLen - 1] == 'L');
    if (isFloat) {
        t.primitiveType = TOKEN_FLOAT;
    } else {
        t.primitiveType = TOKEN_DOUBLE;
        if (isLong) {
            t.isLong = true;
        }
    }
    t.isComplex = hasImag;
    t.isImaginary = hasImag;
    return t;
}
static ParsedType cg_make_int_literal_type(CodegenContext* ctx, const char* text) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_PRIMITIVE;
    t.primitiveType = TOKEN_INT;

    IntegerLiteralInfo info;
    const TargetLayout* tl = cg_context_get_target_layout(ctx);
    unsigned intBits = (unsigned)((tl && tl->intBits) ? tl->intBits : 32);
    if (parse_integer_literal_info(text, tl, &info) && info.ok) {
        t.isUnsigned = info.isUnsigned;
        if (info.bits > intBits) {
            t.primitiveType = TOKEN_LONG;
        }
    }
    return t;
}
static bool cg_binary_op_is_comparison(const char* op) {
    return op &&
           (strcmp(op, "==") == 0 ||
            strcmp(op, "!=") == 0 ||
            strcmp(op, "<") == 0 ||
            strcmp(op, "<=") == 0 ||
            strcmp(op, ">") == 0 ||
            strcmp(op, ">=") == 0);
}
static bool cg_named_type_is_builtin_int128(const ParsedType* type, bool* outIsUnsigned) {
    if (!type || type->kind != TYPE_NAMED || !type->userTypeName) {
        return false;
    }
    if (strcmp(type->userTypeName, "__int128_t") == 0 ||
        strcmp(type->userTypeName, "__int128") == 0) {
        if (outIsUnsigned) *outIsUnsigned = false;
        return true;
    }
    if (strcmp(type->userTypeName, "__uint128_t") == 0 ||
        strcmp(type->userTypeName, "__uint128") == 0) {
        if (outIsUnsigned) *outIsUnsigned = true;
        return true;
    }
    return false;
}
static bool cg_parsed_type_is_pointer(const ParsedType* type) {
    if (!type) return false;
    if (type->isFunctionPointer) return true;
    return type->pointerDepth > 0;
}
static bool cg_named_type_has_surface_derivations(const ParsedType* type);
static const ParsedType* cg_resolve_typedef_parsed(CodegenContext* ctx, const ParsedType* type) {
    if (!ctx || !type || type->kind != TYPE_NAMED || !type->userTypeName) {
        return type;
    }
    if (cg_named_type_has_surface_derivations(type)) {
        return type;
    }
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (!cache) return type;
    CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, type->userTypeName);
    if (info) {
        return &info->parsedType;
    }
    {
        const SemanticModel* model = cg_context_get_semantic_model(ctx);
        if (model) {
            const Symbol* sym = semanticModelLookupGlobal(model, type->userTypeName);
            if (sym && sym->kind == SYMBOL_TYPEDEF) {
                return &sym->type;
            }
        }
    }
    return type;
}
static const ParsedType* cg_resolve_typedef_chain(CodegenContext* ctx, const ParsedType* type) {
    const ParsedType* current = type;
    int guard = 0;
    while (current &&
           current->kind == TYPE_NAMED &&
           !cg_named_type_has_surface_derivations(current) &&
           guard < 64) {
        const ParsedType* next = cg_resolve_typedef_parsed(ctx, current);
        if (!next || next == current) {
            break;
        }
        current = next;
        guard++;
    }
    return current ? current : type;
}
static bool cg_named_type_has_surface_derivations(const ParsedType* type) {
    if (!type || type->kind != TYPE_NAMED) {
        return false;
    }
    return type->derivationCount > 0 || type->pointerDepth > 0 || type->isFunctionPointer;
}

static const ParsedType* cg_resolve_call_surface_type(CodegenContext* ctx, const ParsedType* type) {
    if (cg_named_type_has_surface_derivations(type)) {
        return type;
    }
    return cg_resolve_typedef_chain(ctx, type);
}

static const ParsedType* cg_lookup_function_symbol_return_type(CodegenContext* ctx, ASTNode* callee) {
    if (!ctx || !callee || callee->type != AST_IDENTIFIER) {
        if (!ctx || !callee) {
            return NULL;
        }
        if (callee->type == AST_TERNARY_EXPRESSION) {
            const ParsedType* lhs =
                cg_lookup_function_symbol_return_type(ctx, callee->ternaryExpr.trueExpr);
            const ParsedType* rhs =
                cg_lookup_function_symbol_return_type(ctx, callee->ternaryExpr.falseExpr);
            if (lhs && rhs && parsedTypesStructurallyEqual(lhs, rhs)) {
                return lhs;
            }
        }
        return NULL;
    }
    const char* name = callee->valueNode.value;
    if (!name) {
        return NULL;
    }
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (!model) {
        return NULL;
    }
    const Symbol* sym = semanticModelLookupGlobal(model, name);
    if (!sym || sym->kind != SYMBOL_FUNCTION) {
        return NULL;
    }
    static ParsedType symbolReturnType;
    parsedTypeFree(&symbolReturnType);
    symbolReturnType = parsedTypeClone(&sym->type);
    if (symbolReturnType.kind == TYPE_INVALID) {
        parsedTypeFree(&symbolReturnType);
        return NULL;
    }
    cg_clear_function_pointer_metadata(&symbolReturnType);
    return &symbolReturnType;
}

static bool cg_parsed_type_is_complex(const ParsedType* type) {
    return type && (type->isComplex || type->isImaginary);
}

static bool cg_kind_is_aggregate_like(LLVMTypeKind kind) {
    return kind == LLVMStructTypeKind || kind == LLVMArrayTypeKind;
}

static bool cg_kind_is_abi_pack_scalar(LLVMTypeKind kind) {
    return kind == LLVMIntegerTypeKind || kind == LLVMArrayTypeKind;
}

static LLVMValueRef cg_reinterpret_via_stack(CodegenContext* ctx,
                                             LLVMValueRef value,
                                             LLVMTypeRef targetType,
                                             const char* tag) {
    if (!ctx || !value || !targetType || !ctx->module) {
        return value;
    }
    LLVMTypeRef sourceType = LLVMTypeOf(value);
    if (!sourceType) {
        return value;
    }

    LLVMTargetDataRef td = LLVMGetModuleDataLayout(ctx->module);
    if (!td) {
        return value;
    }

    uint64_t srcSize = LLVMABISizeOfType(td, sourceType);
    uint64_t dstSize = LLVMABISizeOfType(td, targetType);
    if (srcSize == 0 || dstSize == 0 || srcSize != dstSize) {
        return value;
    }

    LLVMValueRef slot = cg_build_entry_alloca(ctx, targetType, "cast.repack.slot");
    if (!slot) {
        return value;
    }

    if (!cg_kind_is_aggregate_like(LLVMGetTypeKind(targetType))) {
        LLVMBuildStore(ctx->builder, LLVMConstNull(targetType), slot);
    }

    LLVMTypeRef sourcePtrType = LLVMPointerType(sourceType, 0);
    LLVMValueRef sourceAddr = LLVMBuildBitCast(ctx->builder, slot, sourcePtrType, "cast.repack.addr");
    LLVMBuildStore(ctx->builder, value, sourceAddr);
    return LLVMBuildLoad2(ctx->builder, targetType, slot, tag);
}

static LLVMTypeRef cg_complex_element_type(CodegenContext* ctx, const ParsedType* type) {
    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    if (!type) {
        return LLVMDoubleTypeInContext(llvmCtx);
    }
    switch (type->primitiveType) {
        case TOKEN_FLOAT:
            return LLVMFloatTypeInContext(llvmCtx);
        case TOKEN_DOUBLE:
        default:
            if (type->isLong) {
                const TargetLayout* tl = cg_context_get_target_layout(ctx);
                if (tl && tl->longDoubleBits == 128) {
                    return LLVMFP128TypeInContext(llvmCtx);
                }
                if (tl && tl->longDoubleBits == 64) {
                    return LLVMDoubleTypeInContext(llvmCtx);
                }
                LLVMTypeRef fp80 = LLVMX86FP80TypeInContext(llvmCtx);
                if (fp80) {
                    return fp80;
                }
            }
            return LLVMDoubleTypeInContext(llvmCtx);
    }
}

static LLVMValueRef cg_build_complex(CodegenContext* ctx,
                                     LLVMValueRef real,
                                     LLVMValueRef imag,
                                     LLVMTypeRef complexType,
                                     const char* nameHint) {
    if (!ctx || !complexType) return NULL;
    LLVMValueRef tmp = LLVMGetUndef(complexType);
    tmp = LLVMBuildInsertValue(ctx->builder, tmp, real, 0, nameHint ? nameHint : "complex.r");
    tmp = LLVMBuildInsertValue(ctx->builder, tmp, imag, 1, nameHint ? nameHint : "complex.i");
    return tmp;
}

static CGValueCategory cg_classify_parsed_type(const ParsedType* type) {
    if (!type) {
        return CG_VALUE_UNKNOWN;
    }

    if (cg_parsed_type_is_pointer(type)) {
        return CG_VALUE_POINTER;
    }

    switch (type->kind) {
        case TYPE_STRUCT:
        case TYPE_UNION:
            return CG_VALUE_AGGREGATE;
        case TYPE_ENUM:
            return CG_VALUE_SIGNED_INT;
        case TYPE_PRIMITIVE:
            switch (type->primitiveType) {
                case TOKEN_BOOL:
                    return CG_VALUE_BOOL;
                case TOKEN_FLOAT:
                case TOKEN_DOUBLE:
                    return CG_VALUE_FLOAT;
                case TOKEN_UNSIGNED:
                    return CG_VALUE_UNSIGNED_INT;
                case TOKEN_SIGNED:
                case TOKEN_INT:
                case TOKEN_LONG:
                case TOKEN_SHORT:
                case TOKEN_CHAR:
                    return type->isUnsigned ? CG_VALUE_UNSIGNED_INT : CG_VALUE_SIGNED_INT;
                default:
                    break;
            }
            if (type->isUnsigned) {
                return CG_VALUE_UNSIGNED_INT;
            }
            return CG_VALUE_SIGNED_INT;
        case TYPE_NAMED:
            {
                bool isUnsigned128 = false;
                if (cg_named_type_is_builtin_int128(type, &isUnsigned128)) {
                    return isUnsigned128 ? CG_VALUE_UNSIGNED_INT : CG_VALUE_SIGNED_INT;
                }
            }
            switch (type->tag) {
                case TAG_STRUCT:
                case TAG_UNION:
                    return CG_VALUE_AGGREGATE;
                case TAG_ENUM:
                    return CG_VALUE_SIGNED_INT;
                default:
                    break;
            }
            return type->isUnsigned ? CG_VALUE_UNSIGNED_INT : CG_VALUE_SIGNED_INT;
        case TYPE_INVALID:
        default:
            break;
    }

    if (type->isUnsigned) {
        return CG_VALUE_UNSIGNED_INT;
    }
    return CG_VALUE_SIGNED_INT;
}

static bool cg_is_unsigned_category(CGValueCategory category) {
    return category == CG_VALUE_UNSIGNED_INT || category == CG_VALUE_BOOL;
}

static bool cg_kind_is_float(LLVMTypeKind kind) {
    switch (kind) {
        case LLVMHalfTypeKind:
        case LLVMFloatTypeKind:
        case LLVMDoubleTypeKind:
        case LLVMFP128TypeKind:
        case LLVMX86_FP80TypeKind:
            return true;
        default:
            return false;
    }
}

static unsigned cg_float_rank(LLVMTypeKind kind) {
    switch (kind) {
        case LLVMHalfTypeKind: return 1;
        case LLVMFloatTypeKind: return 2;
        case LLVMDoubleTypeKind: return 3;
        case LLVMX86_FP80TypeKind: return 4;
        case LLVMFP128TypeKind: return 5;
        default: return 0;
    }
}

static bool cg_should_treat_as_unsigned(CodegenContext* ctx,
                                        const ParsedType* parsedType,
                                        LLVMTypeRef llvmType) {
    if (parsedType) {
        const ParsedType* resolved = cg_resolve_typedef_chain(ctx, parsedType);
        CGValueCategory category = cg_classify_parsed_type(resolved ? resolved : parsedType);
        return cg_is_unsigned_category(category);
    }

    LLVMTypeKind kind = LLVMGetTypeKind(llvmType);
    if (kind == LLVMIntegerTypeKind && LLVMGetIntTypeWidth(llvmType) == 1) {
        return true;
    }
    return false;
}

LLVMTypeRef cg_merge_types_for_phi(CodegenContext* ctx,
                                   const ParsedType* a,
                                   const ParsedType* b,
                                   LLVMValueRef aVal,
                                   LLVMValueRef bVal) {
    if (!ctx) return NULL;
    if (aVal && bVal) {
        LLVMTypeRef aTy = LLVMTypeOf(aVal);
        LLVMTypeRef bTy = LLVMTypeOf(bVal);
        LLVMTypeKind aKind = aTy ? LLVMGetTypeKind(aTy) : LLVMVoidTypeKind;
        LLVMTypeKind bKind = bTy ? LLVMGetTypeKind(bTy) : LLVMVoidTypeKind;
        bool aFloat = cg_kind_is_float(aKind);
        bool bFloat = cg_kind_is_float(bKind);
        if (aFloat || bFloat) {
            unsigned aRank = cg_float_rank(aKind);
            unsigned bRank = cg_float_rank(bKind);
            if (aRank >= bRank) {
                return aTy ? aTy : bTy;
            }
            return bTy ? bTy : aTy;
        }
        if (aTy && bTy &&
            aKind == LLVMIntegerTypeKind &&
            bKind == LLVMIntegerTypeKind) {
            unsigned aBits = LLVMGetIntTypeWidth(aTy);
            unsigned bBits = LLVMGetIntTypeWidth(bTy);
            return (aBits >= bBits) ? aTy : bTy;
        }
    }
    if (!a && !b) {
        if (aVal) return LLVMTypeOf(aVal);
        if (bVal) return LLVMTypeOf(bVal);
        return NULL;
    }
    if (!a) return bVal ? LLVMTypeOf(bVal) : NULL;
    if (!b) return aVal ? LLVMTypeOf(aVal) : NULL;
    CGValueCategory aCat = cg_classify_parsed_type(a);
    CGValueCategory bCat = cg_classify_parsed_type(b);
    if (cg_is_unsigned_category(aCat) && !cg_is_unsigned_category(bCat)) {
        return LLVMTypeOf(aVal);
    }
    if (cg_is_unsigned_category(bCat) && !cg_is_unsigned_category(aCat)) {
        return LLVMTypeOf(bVal);
    }
    return LLVMTypeOf(aVal);
}

static LLVMIntPredicate cg_select_signed_pred(const char* op) {
    if (strcmp(op, "==") == 0) return LLVMIntEQ;
    if (strcmp(op, "!=") == 0) return LLVMIntNE;
    if (strcmp(op, "<") == 0) return LLVMIntSLT;
    if (strcmp(op, "<=") == 0) return LLVMIntSLE;
    if (strcmp(op, ">") == 0) return LLVMIntSGT;
    if (strcmp(op, ">=") == 0) return LLVMIntSGE;
    return LLVMIntEQ;
}

static LLVMIntPredicate cg_select_unsigned_pred(const char* op) {
    if (strcmp(op, "==") == 0) return LLVMIntEQ;
    if (strcmp(op, "!=") == 0) return LLVMIntNE;
    if (strcmp(op, "<") == 0) return LLVMIntULT;
    if (strcmp(op, "<=") == 0) return LLVMIntULE;
    if (strcmp(op, ">") == 0) return LLVMIntUGT;
    if (strcmp(op, ">=") == 0) return LLVMIntUGE;
    return LLVMIntEQ;
}

const ParsedType* cg_resolve_expression_type(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node) return NULL;
    switch (node->type) {
        case AST_IDENTIFIER: {
            const char* name = node->valueNode.value;
            if (!name) {
                fprintf(stderr, "Error: identifier missing name during type resolution (line %d)\n", node->line);
                return NULL;
            }
            NamedValue* entry = cg_scope_lookup(ctx->currentScope, name);
            if (getenv("DEBUG_PTR_ARITH")) {
                fprintf(stderr,
                        "[ptr-arith] resolve ident '%s' entry=%p parsed=%p\n",
                        name,
                        (void*)entry,
                        (void*)(entry ? entry->parsedType : NULL));
            }
            if (entry && entry->parsedType) {
                return cg_resolve_typedef_chain(ctx, entry->parsedType);
            }
            const SemanticModel* model = cg_context_get_semantic_model(ctx);
            if (model) {
                const Symbol* sym = semanticModelLookupGlobal(model, name);
                if (sym) {
                    return cg_resolve_typedef_chain(ctx, &sym->type);
                }
            }
            return NULL;
        }
        case AST_NUMBER_LITERAL:
            if (cg_literal_looks_float(node->valueNode.value)) {
                static ParsedType litTypes[4];
                static int litIndex = 0;
                ParsedType* slot = &litTypes[litIndex];
                litIndex = (litIndex + 1) % 4;
                parsedTypeFree(slot);
                *slot = cg_make_float_literal_type(node->valueNode.value);
                return slot;
            }
            {
                static ParsedType litTypes[4];
                static int litIndex = 0;
                ParsedType* slot = &litTypes[litIndex];
                litIndex = (litIndex + 1) % 4;
                parsedTypeFree(slot);
                *slot = cg_make_int_literal_type(ctx, node->valueNode.value);
                return slot;
            }
        case AST_CHAR_LITERAL:
            return cg_builtin_signed_int_type();
        case AST_SIZEOF:
        case AST_ALIGNOF:
            return cg_builtin_unsigned_int_type();
        case AST_CAST_EXPRESSION:
            return &node->castExpr.castType;
        case AST_ASSIGNMENT:
            return cg_resolve_expression_type(ctx, node->assignment.target);
        case AST_TERNARY_EXPRESSION: {
            const ParsedType* tType = cg_resolve_expression_type(ctx, node->ternaryExpr.trueExpr);
            const ParsedType* fType = cg_resolve_expression_type(ctx, node->ternaryExpr.falseExpr);
            if (!tType && !fType) return NULL;
            if (!tType) return fType;
            if (!fType) return tType;
            CGValueCategory tCat = cg_classify_parsed_type(tType);
            CGValueCategory fCat = cg_classify_parsed_type(fType);
            if (cg_is_unsigned_category(tCat)) return tType;
            if (cg_is_unsigned_category(fCat)) return fType;
            return tType;
        }
        case AST_BINARY_EXPRESSION: {
            const char* op = node->expr.op;
            if (!op) return NULL;
            if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0 || cg_binary_op_is_comparison(op)) {
                return cg_builtin_signed_int_type();
            }
            const ParsedType* left = cg_resolve_expression_type(ctx, node->expr.left);
            const ParsedType* right = cg_resolve_expression_type(ctx, node->expr.right);
            if ((strcmp(op, "+") == 0 || strcmp(op, "-") == 0) && left && cg_parsed_type_is_pointer(left)) {
                return left;
            }
            if (strcmp(op, "+") == 0 && right && cg_parsed_type_is_pointer(right)) {
                return right;
            }
            if (!left) return right;
            if (!right) return left;
            CGValueCategory lcat = cg_classify_parsed_type(left);
            CGValueCategory rcat = cg_classify_parsed_type(right);
            if (cg_is_unsigned_category(lcat)) return left;
            if (cg_is_unsigned_category(rcat)) return right;
            return left;
        }
        case AST_COMPOUND_LITERAL:
            return &node->compoundLiteral.literalType;
        case AST_UNARY_EXPRESSION: {
            const char* op = node->expr.op;
            if (!op) return cg_resolve_expression_type(ctx, node->expr.left);
            if (strcmp(op, "!") == 0) {
                return cg_builtin_signed_int_type();
            }
            if (strcmp(op, "*") == 0) {
                const ParsedType* operand = cg_resolve_expression_type(ctx, node->expr.left);
                if (operand &&
                    !cg_parsed_type_has_pointer_layer(operand) &&
                    parsedTypeIsDirectArray(operand)) {
                    static ParsedType arrayElement;
                    parsedTypeFree(&arrayElement);
                    arrayElement = parsedTypeArrayElementType(operand);
                    if (arrayElement.kind != TYPE_INVALID) {
                        return &arrayElement;
                    }
                    parsedTypeFree(&arrayElement);
                }
                ParsedType operandCopy = parsedTypeClone(operand);
                static ParsedType pointed;
                parsedTypeFree(&pointed);
                pointed = parsedTypePointerTargetType(&operandCopy);
                parsedTypeFree(&operandCopy);
                if (pointed.kind != TYPE_INVALID) {
                    return &pointed;
                }
                parsedTypeFree(&pointed);
                return operand;
            }
            if (strcmp(op, "&") == 0) {
                const ParsedType* operand = cg_resolve_expression_type(ctx, node->expr.left);
                static ParsedType addrType;
                parsedTypeFree(&addrType);
                addrType = parsedTypeClone(operand);
                if (addrType.kind != TYPE_INVALID) {
                    parsedTypeAddPointerDepth(&addrType, 1);
                    return &addrType;
                }
                return operand;
            }
            return cg_resolve_expression_type(ctx, node->expr.left);
        }
        case AST_POINTER_ACCESS:
        case AST_DOT_ACCESS:
        case AST_STRUCT_FIELD_ACCESS: {
            ASTNode* baseNode = (node->type == AST_STRUCT_FIELD_ACCESS)
                ? node->structFieldAccess.structInstance
                : node->memberAccess.base;
            const char* fieldName = (node->type == AST_STRUCT_FIELD_ACCESS)
                ? node->structFieldAccess.fieldName
                : node->memberAccess.field;
            const ParsedType* base = cg_resolve_expression_type(ctx, baseNode);
            if (!base) return NULL;
            ParsedType baseCopy = parsedTypeClone(base);
            if (node->type == AST_POINTER_ACCESS) {
                ParsedType target = parsedTypePointerTargetType(&baseCopy);
                parsedTypeFree(&baseCopy);
                baseCopy = target;
            }
            const ParsedType* structParsed = &baseCopy;
            if (baseCopy.kind == TYPE_NAMED) {
                structParsed = cg_resolve_typedef_parsed(ctx, &baseCopy);
            }
            if (structParsed &&
                (structParsed->kind == TYPE_STRUCT || structParsed->kind == TYPE_UNION) &&
                structParsed->userTypeName) {
                CGTypeCache* cache = cg_context_get_type_cache(ctx);
                CGStructLLVMInfo* info = cache
                    ? cg_type_cache_get_struct_info(cache, structParsed->userTypeName)
                    : NULL;
                if (info && fieldName) {
                    for (size_t i = 0; i < info->fieldCount; ++i) {
                        if (info->fields[i].name &&
                            fieldName &&
                            strcmp(info->fields[i].name, fieldName) == 0) {
                            parsedTypeFree(&baseCopy);
                            return &info->fields[i].parsedType;
                        }
                    }
                }
                if (ctx && ctx->semanticModel && fieldName) {
                    CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                    CCTagKind tagKind = (structParsed->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                    ASTNode* def = cctx ? cc_tag_definition(cctx, tagKind, structParsed->userTypeName) : NULL;
                    if (def && (def->type == AST_STRUCT_DEFINITION || def->type == AST_UNION_DEFINITION)) {
                        for (size_t f = 0; f < def->structDef.fieldCount; ++f) {
                            ASTNode* fieldDecl = def->structDef.fields ? def->structDef.fields[f] : NULL;
                            if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
                            for (size_t v = 0; v < fieldDecl->varDecl.varCount; ++v) {
                                ASTNode* nameNode = fieldDecl->varDecl.varNames[v];
                                const char* declFieldName = (nameNode && nameNode->type == AST_IDENTIFIER)
                                    ? nameNode->valueNode.value
                                    : NULL;
                                if (declFieldName && strcmp(declFieldName, fieldName) == 0) {
                                    parsedTypeFree(&baseCopy);
                                    return astVarDeclTypeAt(fieldDecl, v);
                                }
                            }
                        }
                    }
                }
            }
            if (structParsed && fieldName && ctx->semanticModel) {
                Scope* semanticScope = semanticModelGetGlobalScope(ctx->semanticModel);
                if (semanticScope) {
                    TypeInfo baseInfo = typeInfoFromParsedType(structParsed, semanticScope);
                    const ParsedType* resolvedField =
                        analyzeExprLookupFieldType(&baseInfo, fieldName, semanticScope);
                    if (resolvedField) {
                        parsedTypeFree(&baseCopy);
                        return resolvedField;
                    }
                }
            }
            if (structParsed && fieldName && structParsed->inlineStructOrUnionDef) {
                ASTNode* def = structParsed->inlineStructOrUnionDef;
                if (def && (def->type == AST_STRUCT_DEFINITION || def->type == AST_UNION_DEFINITION)) {
                    for (size_t f = 0; f < def->structDef.fieldCount; ++f) {
                        ASTNode* fieldDecl = def->structDef.fields ? def->structDef.fields[f] : NULL;
                        if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
                        for (size_t v = 0; v < fieldDecl->varDecl.varCount; ++v) {
                            ASTNode* nameNode = fieldDecl->varDecl.varNames[v];
                            const char* declFieldName = (nameNode && nameNode->type == AST_IDENTIFIER)
                                ? nameNode->valueNode.value
                                : NULL;
                            if (declFieldName && strcmp(declFieldName, fieldName) == 0) {
                                parsedTypeFree(&baseCopy);
                                return astVarDeclTypeAt(fieldDecl, v);
                            }
                        }
                    }
                }
            }
            parsedTypeFree(&baseCopy);
            return base;
        }
        case AST_POINTER_DEREFERENCE: {
            const ParsedType* base = cg_resolve_expression_type(ctx, node->pointerDeref.pointer);
            static ParsedType pointed;
            parsedTypeFree(&pointed);
            pointed = parsedTypePointerTargetType(base);
            if (pointed.kind != TYPE_INVALID) {
                return &pointed;
            }
            parsedTypeFree(&pointed);
            return base;
        }
        case AST_FUNCTION_CALL: {
            ASTNode* callee = node->functionCall.callee;
            const ParsedType* symbolReturn = cg_lookup_function_symbol_return_type(ctx, callee);
            if (symbolReturn) {
                return symbolReturn;
            }
            const ParsedType* calleeType = cg_resolve_expression_type(ctx, callee);
            if (!calleeType && callee && callee->type == AST_IDENTIFIER) {
                const SemanticModel* model = cg_context_get_semantic_model(ctx);
                if (model) {
                    const Symbol* sym = semanticModelLookupGlobal(model, callee->valueNode.value);
                    if (sym) {
                        calleeType = &sym->type;
                    }
                }
            }
            const ParsedType* resolvedCallee = cg_resolve_call_surface_type(ctx, calleeType);
            static ParsedType retType;
            parsedTypeFree(&retType);
            retType = parsedTypeFunctionReturnType(resolvedCallee);
            if (retType.kind == TYPE_INVALID && resolvedCallee && resolvedCallee->isFunctionPointer) {
                ParsedType pointed = parsedTypePointerTargetType(resolvedCallee);
                if (pointed.kind != TYPE_INVALID) {
                    parsedTypeFree(&retType);
                    retType = pointed;
                } else {
                    parsedTypeFree(&pointed);
                }
            }
            cg_clear_function_pointer_metadata(&retType);
            if (retType.kind != TYPE_INVALID) {
                return &retType;
            }
            parsedTypeFree(&retType);
            return calleeType;
        }
        case AST_ARRAY_ACCESS: {
            const ParsedType* base = cg_resolve_expression_type(ctx, node->arrayAccess.array);
            static ParsedType cachedSlots[4];
            static int cachedIndex = 0;
            ParsedType* cached = &cachedSlots[cachedIndex];
            cachedIndex = (cachedIndex + 1) % 4;
            parsedTypeFree(cached);
            cached->kind = TYPE_INVALID;
            if (!base) return NULL;
            if (parsedTypeIsDirectArray(base)) {
                *cached = parsedTypeArrayElementType(base);
                return cached;
            }
            ParsedType pointed = parsedTypePointerTargetType(base);
            if (pointed.kind != TYPE_INVALID) {
                *cached = pointed;
                return cached;
            }
            parsedTypeFree(&pointed);
            return NULL;
        }
        default:
            break;
    }
    return NULL;
}

const ParsedType* cg_refine_function_call_result_type(CodegenContext* ctx, ASTNode* callNode) {
    if (!ctx || !callNode || callNode->type != AST_FUNCTION_CALL) {
        return NULL;
    }
    const ParsedType* symbolReturn =
        cg_lookup_function_symbol_return_type(ctx, callNode->functionCall.callee);
    if (symbolReturn) {
        return symbolReturn;
    }
    const ParsedType* calleeType = cg_resolve_expression_type(ctx, callNode->functionCall.callee);
    if (!calleeType) {
        return NULL;
    }
    const ParsedType* resolvedCallee = cg_resolve_call_surface_type(ctx, calleeType);

    static ParsedType refined;
    parsedTypeFree(&refined);
    refined = parsedTypeFunctionReturnType(resolvedCallee);
    if (refined.kind == TYPE_INVALID && resolvedCallee && resolvedCallee->isFunctionPointer) {
        ParsedType pointed = parsedTypePointerTargetType(resolvedCallee);
        if (pointed.kind != TYPE_INVALID) {
            parsedTypeFree(&refined);
            refined = pointed;
        } else {
            parsedTypeFree(&pointed);
        }
    }
    cg_clear_function_pointer_metadata(&refined);
    if (refined.kind == TYPE_INVALID) {
        parsedTypeFree(&refined);
        return NULL;
    }

    size_t funcDerivs = parsedTypeCountDerivationsOfKind(&refined, TYPE_DERIVATION_FUNCTION);
    size_t ptrDerivs = parsedTypeCountDerivationsOfKind(&refined, TYPE_DERIVATION_POINTER);
    if (funcDerivs > 0 && ptrDerivs == 0 && refined.pointerDepth == 0) {
        ParsedType next = parsedTypeFunctionReturnType(&refined);
        if (next.kind != TYPE_INVALID) {
            parsedTypeFree(&refined);
            refined = next;
        } else {
            parsedTypeFree(&next);
        }
    }

    return &refined;
}

bool cg_expression_is_unsigned(CodegenContext* ctx, ASTNode* node) {
    const ParsedType* type = cg_resolve_expression_type(ctx, node);
    if (type) {
        const ParsedType* resolved = cg_resolve_typedef_chain(ctx, type);
        CGValueCategory cat = cg_classify_parsed_type(resolved);
        return cg_is_unsigned_category(cat);
    }
    return false;
}

LLVMIntPredicate cg_select_int_predicate(const char* op, bool preferUnsigned) {
    if (preferUnsigned) {
        return cg_select_unsigned_pred(op ? op : "==");
    }
    return cg_select_signed_pred(op ? op : "==");
}

LLVMValueRef cg_build_truthy(CodegenContext* ctx,
                             LLVMValueRef value,
                             const ParsedType* parsedType,
                             const char* nameHint) {
    if (!ctx || !value) {
        return NULL;
    }

    const char* finalName = nameHint ? nameHint : "truthy";
    CGValueCategory category = cg_classify_parsed_type(parsedType);
    LLVMTypeRef valueType = LLVMTypeOf(value);

    switch (category) {
        case CG_VALUE_POINTER:
            if (LLVMGetTypeKind(valueType) == LLVMPointerTypeKind) {
                LLVMValueRef nullPtr = LLVMConstPointerNull(valueType);
                return LLVMBuildICmp(ctx->builder, LLVMIntNE, value, nullPtr, finalName);
            }
            break;
        case CG_VALUE_BOOL:
            if (LLVMGetTypeKind(valueType) == LLVMIntegerTypeKind &&
                LLVMGetIntTypeWidth(valueType) == 1) {
                return value;
            }
            break;
        case CG_VALUE_FLOAT: {
            if (LLVMGetTypeKind(valueType) == LLVMFloatTypeKind ||
                LLVMGetTypeKind(valueType) == LLVMDoubleTypeKind ||
                LLVMGetTypeKind(valueType) == LLVMFP128TypeKind ||
                LLVMGetTypeKind(valueType) == LLVMHalfTypeKind) {
                LLVMValueRef zero = LLVMConstNull(valueType);
                return LLVMBuildFCmp(ctx->builder, LLVMRealUNE, value, zero, finalName);
            }
            break;
        }
        case CG_VALUE_UNSIGNED_INT:
        case CG_VALUE_SIGNED_INT:
        case CG_VALUE_UNKNOWN:
        case CG_VALUE_AGGREGATE:
        default:
            break;
    }

    LLVMTypeRef kindType = valueType;
    if (LLVMGetTypeKind(kindType) == LLVMIntegerTypeKind) {
        LLVMValueRef zero = LLVMConstInt(kindType, 0, 0);
        return LLVMBuildICmp(ctx->builder, LLVMIntNE, value, zero, finalName);
    }

    if (LLVMGetTypeKind(kindType) == LLVMPointerTypeKind) {
        LLVMValueRef nullPtr = LLVMConstPointerNull(kindType);
        return LLVMBuildICmp(ctx->builder, LLVMIntNE, value, nullPtr, finalName);
    }

    if (LLVMGetTypeKind(kindType) == LLVMFloatTypeKind ||
        LLVMGetTypeKind(kindType) == LLVMDoubleTypeKind ||
        LLVMGetTypeKind(kindType) == LLVMFP128TypeKind ||
        LLVMGetTypeKind(kindType) == LLVMHalfTypeKind) {
        LLVMValueRef zero = LLVMConstNull(kindType);
        return LLVMBuildFCmp(ctx->builder, LLVMRealUNE, value, zero, finalName);
    }

    return value;
}

LLVMValueRef cg_cast_value(CodegenContext* ctx,
                           LLVMValueRef value,
                           LLVMTypeRef targetType,
                           const ParsedType* fromParsed,
                           const ParsedType* toParsed,
                           const char* nameHint) {
    if (!ctx || !value || !targetType) {
        return value;
    }

    LLVMTypeRef sourceType = LLVMTypeOf(value);
    if (sourceType == targetType) {
        return value;
    }

    const char* tag = nameHint ? nameHint : "casttmp";
    LLVMTypeKind srcKind = LLVMGetTypeKind(sourceType);
    LLVMTypeKind dstKind = LLVMGetTypeKind(targetType);

    bool srcUnsigned = cg_should_treat_as_unsigned(ctx, fromParsed, sourceType);
    bool dstUnsigned = cg_should_treat_as_unsigned(ctx, toParsed, targetType);

    if (dstKind == LLVMIntegerTypeKind && LLVMGetIntTypeWidth(targetType) == 1) {
        if (srcKind == LLVMIntegerTypeKind) {
            if (LLVMGetIntTypeWidth(sourceType) == 1) {
                return value;
            }
            LLVMValueRef zero = LLVMConstInt(sourceType, 0, 0);
            return LLVMBuildICmp(ctx->builder, LLVMIntNE, value, zero, tag);
        }
        if (srcKind == LLVMPointerTypeKind) {
            LLVMValueRef nullPtr = LLVMConstPointerNull(sourceType);
            return LLVMBuildICmp(ctx->builder, LLVMIntNE, value, nullPtr, tag);
        }
        if (cg_kind_is_float(srcKind)) {
            LLVMValueRef zero = LLVMConstNull(sourceType);
            return LLVMBuildFCmp(ctx->builder, LLVMRealUNE, value, zero, tag);
        }
    }

    if (cg_parsed_type_is_complex(toParsed)) {
        LLVMTypeRef elemTy = cg_complex_element_type(ctx, toParsed);
        LLVMValueRef realVal = NULL;
        LLVMValueRef imagVal = NULL;
        if (cg_parsed_type_is_complex(fromParsed) && srcKind == LLVMStructTypeKind) {
            LLVMValueRef realPart = LLVMBuildExtractValue(ctx->builder, value, 0, "complex.real");
            LLVMValueRef imagPart = LLVMBuildExtractValue(ctx->builder, value, 1, "complex.imag");
            realVal = cg_cast_value(ctx, realPart, elemTy, fromParsed, NULL, "complex.r.cast");
            imagVal = cg_cast_value(ctx, imagPart, elemTy, fromParsed, NULL, "complex.i.cast");
        } else {
            LLVMValueRef casted = value;
            if (LLVMGetTypeKind(LLVMTypeOf(value)) != LLVMGetTypeKind(elemTy) ||
                LLVMTypeOf(value) != elemTy) {
                casted = cg_cast_value(ctx, value, elemTy, fromParsed, NULL, "complex.cast");
            }
            if (fromParsed && fromParsed->isImaginary) {
                realVal = LLVMConstNull(elemTy);
                imagVal = casted;
            } else {
                realVal = casted;
                imagVal = LLVMConstNull(elemTy);
            }
        }
        return cg_build_complex(ctx, realVal, imagVal, targetType, "complex.pack");
    }

    if (cg_parsed_type_is_complex(fromParsed) && srcKind == LLVMStructTypeKind) {
        LLVMValueRef realPart = LLVMBuildExtractValue(ctx->builder, value, 0, "complex.real");
        if (fromParsed && fromParsed->isImaginary) {
            realPart = LLVMBuildExtractValue(ctx->builder, value, 1, "complex.imag");
        }
        return cg_cast_value(ctx, realPart, targetType, NULL, toParsed, "complex.to.scalar");
    }

    if ((cg_kind_is_aggregate_like(srcKind) && cg_kind_is_abi_pack_scalar(dstKind)) ||
        (cg_kind_is_abi_pack_scalar(srcKind) && cg_kind_is_aggregate_like(dstKind))) {
        LLVMValueRef repacked = cg_reinterpret_via_stack(ctx, value, targetType, tag);
        if (repacked && LLVMTypeOf(repacked) == targetType) {
            return repacked;
        }
    }

    if (srcKind == LLVMIntegerTypeKind && dstKind == LLVMIntegerTypeKind) {
        unsigned srcBits = LLVMGetIntTypeWidth(sourceType);
        unsigned dstBits = LLVMGetIntTypeWidth(targetType);
        if (srcBits == dstBits) {
            return LLVMBuildBitCast(ctx->builder, value, targetType, tag);
        } else if (srcBits < dstBits) {
            return srcUnsigned
                ? LLVMBuildZExt(ctx->builder, value, targetType, tag)
                : LLVMBuildSExt(ctx->builder, value, targetType, tag);
        } else {
            return LLVMBuildTrunc(ctx->builder, value, targetType, tag);
        }
    }

    if (cg_kind_is_float(srcKind) && cg_kind_is_float(dstKind)) {
        unsigned srcRank = cg_float_rank(srcKind);
        unsigned dstRank = cg_float_rank(dstKind);
        if (srcRank < dstRank) {
            return LLVMBuildFPExt(ctx->builder, value, targetType, tag);
        }
        return LLVMBuildFPTrunc(ctx->builder, value, targetType, tag);
    }

    if (cg_kind_is_float(srcKind) && dstKind == LLVMIntegerTypeKind) {
        return dstUnsigned
            ? LLVMBuildFPToUI(ctx->builder, value, targetType, tag)
            : LLVMBuildFPToSI(ctx->builder, value, targetType, tag);
    }

    if (srcKind == LLVMIntegerTypeKind && cg_kind_is_float(dstKind)) {
        return srcUnsigned
            ? LLVMBuildUIToFP(ctx->builder, value, targetType, tag)
            : LLVMBuildSIToFP(ctx->builder, value, targetType, tag);
    }

    if (srcKind == LLVMPointerTypeKind && dstKind == LLVMPointerTypeKind) {
        return LLVMBuildBitCast(ctx->builder, value, targetType, tag);
    }

    if (srcKind == LLVMPointerTypeKind && dstKind == LLVMIntegerTypeKind) {
        return LLVMBuildPtrToInt(ctx->builder, value, targetType, tag);
    }

    if (srcKind == LLVMIntegerTypeKind && dstKind == LLVMPointerTypeKind) {
        return LLVMBuildIntToPtr(ctx->builder, value, targetType, tag);
    }

    return LLVMBuildBitCast(ctx->builder, value, targetType, tag);
}
