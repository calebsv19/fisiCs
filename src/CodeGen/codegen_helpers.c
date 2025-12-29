#include "codegen_private.h"

#include "codegen_types.h"
#include "Syntax/layout.h"

#include <llvm-c/Core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LLVMTypeRef cg_dereference_ptr_type(CodegenContext* ctx, LLVMTypeRef ptrType, const char* ctxMsg) {
    (void)ctx;
    if (!ptrType || LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
        if (ctxMsg) {
            CG_ERROR("Cannot dereference non-pointer in %s", ctxMsg);
        } else {
            CG_ERROR("Cannot dereference non-pointer");
        }
        return NULL;
    }
    LLVMTypeRef elem = LLVMGetElementType(ptrType);
    if (!elem) {
        if (ctxMsg) {
            CG_ERROR("Opaque pointer encountered in %s", ctxMsg);
        } else {
            CG_ERROR("Opaque pointer encountered");
        }
        return NULL;
    }
    return elem;
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

static bool cg_binary_op_is_comparison(const char* op) {
    return op &&
           (strcmp(op, "==") == 0 ||
            strcmp(op, "!=") == 0 ||
            strcmp(op, "<") == 0 ||
            strcmp(op, "<=") == 0 ||
            strcmp(op, ">") == 0 ||
            strcmp(op, ">=") == 0);
}

static bool cg_parsed_type_is_pointer(const ParsedType* type) {
    if (!type) return false;
    if (type->isFunctionPointer) return true;
    return type->pointerDepth > 0;
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

static bool cg_should_treat_as_unsigned(const ParsedType* parsedType, LLVMTypeRef llvmType) {
    if (parsedType) {
        CGValueCategory category = cg_classify_parsed_type(parsedType);
        return cg_is_unsigned_category(category);
    }

    LLVMTypeKind kind = LLVMGetTypeKind(llvmType);
    if (kind == LLVMIntegerTypeKind && LLVMGetIntTypeWidth(llvmType) == 1) {
        return true;
    }
    return false;
}

LLVMTypeRef cg_get_intptr_type(CodegenContext* ctx) {
    if (ctx && ctx->module) {
        LLVMTargetDataRef layout = LLVMGetModuleDataLayout(ctx->module);
        if (layout) {
            return LLVMIntPtrType(layout);
        }
    }
    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    return llvmCtx ? LLVMInt64TypeInContext(llvmCtx) : LLVMInt64Type();
}

LLVMValueRef cg_widen_bool_to_int(CodegenContext* ctx, LLVMValueRef value, const char* nameHint) {
    LLVMTypeRef type = LLVMTypeOf(value);
    if (type && LLVMGetTypeKind(type) == LLVMIntegerTypeKind && LLVMGetIntTypeWidth(type) == 1) {
        LLVMTypeRef target = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
        return LLVMBuildZExt(ctx->builder, value, target, nameHint ? nameHint : "bool.to.int");
    }
    return value;
}

LLVMTypeRef cg_merge_types_for_phi(CodegenContext* ctx,
                                   const ParsedType* a,
                                   const ParsedType* b,
                                   LLVMValueRef aVal,
                                   LLVMValueRef bVal) {
    if (!ctx) return NULL;
    if (!a && !b) return NULL;
    if (!a) return LLVMTypeOf(bVal);
    if (!b) return LLVMTypeOf(aVal);
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

bool cg_size_align_of_parsed(CodegenContext* ctx,
                             const ParsedType* parsed,
                             uint64_t* outSize,
                             uint32_t* outAlign) {
    if (!ctx || !parsed) return false;
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (!model) return false;
    CompilerContext* cctx = semanticModelGetContext(model);
    struct Scope* gscope = semanticModelGetGlobalScope(model);
    if (!cctx || !gscope) return false;
    ParsedType copy = *parsed;
    size_t sz = 0, al = 0;
    if (!size_align_of_parsed_type(&copy, gscope, &sz, &al)) {
        return false;
    }
    if (outSize) *outSize = (uint64_t)sz;
    if (outAlign) *outAlign = (uint32_t)al;
    return true;
}

bool cg_size_align_for_type(CodegenContext* ctx,
                            const ParsedType* parsed,
                            LLVMTypeRef llvmHint,
                            uint64_t* outSize,
                            uint32_t* outAlign) {
    uint64_t sz = 0;
    uint32_t al = 0;
    bool haveSemantic = cg_size_align_of_parsed(ctx, parsed, &sz, &al);

    LLVMTargetDataRef tdata = ctx && ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
    bool haveLLVM = false;
    uint64_t llvmSize = 0;
    uint32_t llvmAlign = 0;
    if (tdata && llvmHint) {
        llvmSize = LLVMABISizeOfType(tdata, llvmHint);
        llvmAlign = (uint32_t)LLVMABIAlignmentOfType(tdata, llvmHint);
        haveLLVM = true;
    }

    if (haveSemantic) {
        if (haveLLVM && getenv("FISICS_DEBUG_LAYOUT")) {
            if (sz != llvmSize || (al && llvmAlign && al != llvmAlign)) {
                fprintf(stderr,
                        "[layout] semantic vs LLVM mismatch: sz=%llu(%llu) align=%u(%u)\n",
                        (unsigned long long)sz,
                        (unsigned long long)llvmSize,
                        al,
                        llvmAlign);
            }
        }
        if (outSize) *outSize = sz;
        if (outAlign) *outAlign = al ? al : (haveLLVM ? llvmAlign : 1);
        return true;
    }

    if (haveLLVM) {
        if (outSize) *outSize = llvmSize;
        if (outAlign) *outAlign = llvmAlign ? llvmAlign : 1;
        return true;
    }
    return false;
}

LLVMTypeRef cg_element_type_from_pointer(CodegenContext* ctx,
                                         const ParsedType* pointerParsed,
                                         LLVMTypeRef pointerLLVM) {
    if (pointerParsed) {
        ParsedType element = parsedTypePointerTargetType(pointerParsed);
        if (element.kind != TYPE_INVALID) {
            LLVMTypeRef elemType = cg_type_from_parsed(ctx, &element);
            parsedTypeFree(&element);
            if (elemType) {
                return elemType;
            }
        } else {
            parsedTypeFree(&element);
        }
    }
    LLVMTypeRef elem = cg_dereference_ptr_type(ctx, pointerLLVM, "element type lookup");
    if (elem && LLVMGetTypeKind(elem) == LLVMArrayTypeKind) {
        unsigned len = LLVMGetArrayLength(elem);
        LLVMTypeRef inner = LLVMGetElementType(elem);
        if (len == 0 && inner) {
            elem = inner;
        } else {
            elem = inner;
        }
    }
    if (elem && LLVMGetTypeKind(elem) != LLVMVoidTypeKind) {
        return elem;
    }
    return LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx));
}

LLVMTypeRef cg_element_type_hint_from_parsed(CodegenContext* ctx, const ParsedType* parsedType) {
    if (!ctx || !parsedType) {
        return NULL;
    }

    LLVMTypeRef hint = NULL;
    if (parsedTypeIsDirectArray(parsedType)) {
        ParsedType element = parsedTypeArrayElementType(parsedType);
        hint = cg_type_from_parsed(ctx, &element);
        parsedTypeFree(&element);
    } else {
        ParsedType pointed = parsedTypePointerTargetType(parsedType);
        if (pointed.kind != TYPE_INVALID) {
            hint = cg_type_from_parsed(ctx, &pointed);
        }
        parsedTypeFree(&pointed);
    }

    if (hint && LLVMGetTypeKind(hint) == LLVMVoidTypeKind) {
        hint = NULL;
    }
    return hint;
}

LLVMValueRef cg_build_pointer_offset(CodegenContext* ctx,
                                     LLVMValueRef basePtr,
                                     LLVMValueRef offsetValue,
                                     const ParsedType* pointerParsed,
                                     const ParsedType* offsetParsed,
                                     bool isSubtract) {
    if (!ctx || !basePtr || !offsetValue) return NULL;
    (void)offsetParsed;

    LLVMTypeRef ptrType = LLVMTypeOf(basePtr);
    if (!ptrType || LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
        fprintf(stderr, "Error: pointer arithmetic requires pointer base\n");
        return NULL;
    }

    LLVMValueRef index = offsetValue;
    LLVMTypeRef offsetType = LLVMTypeOf(offsetValue);
    if (offsetType && LLVMGetTypeKind(offsetType) == LLVMIntegerTypeKind) {
        LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
        if (offsetType != intptrTy) {
            index = LLVMBuildIntCast2(ctx->builder, offsetValue, intptrTy, 0, "ptr.idx.cast");
        }
    } else if (LLVMGetTypeKind(offsetType) == LLVMPointerTypeKind) {
        fprintf(stderr, "Error: pointer arithmetic with pointer offset unsupported\n");
        return NULL;
    } else {
        fprintf(stderr, "Error: pointer arithmetic offset must be integer\n");
        return NULL;
    }

    if (isSubtract) {
        index = LLVMBuildNeg(ctx->builder, index, "ptr.idx.neg");
    }

    LLVMTypeRef elementType = cg_element_type_from_pointer(ctx, pointerParsed, ptrType);

    return LLVMBuildGEP2(ctx->builder, elementType, basePtr, &index, 1, "ptr.arith");
}

LLVMValueRef cg_build_pointer_difference(CodegenContext* ctx,
                                         LLVMValueRef lhsPtr,
                                         LLVMValueRef rhsPtr,
                                         const ParsedType* lhsParsed) {
    if (!ctx || !lhsPtr || !rhsPtr) return NULL;
    LLVMTypeRef lhsType = LLVMTypeOf(lhsPtr);
    LLVMTypeRef rhsType = LLVMTypeOf(rhsPtr);

    if (!lhsType || !rhsType || LLVMGetTypeKind(lhsType) != LLVMPointerTypeKind ||
        LLVMGetTypeKind(rhsType) != LLVMPointerTypeKind) {
        fprintf(stderr, "Error: pointer difference requires pointer operands\n");
        return NULL;
    }

    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    LLVMValueRef lhsInt = LLVMBuildPtrToInt(ctx->builder, lhsPtr, intptrTy, "ptr.diff.lhs");
    LLVMValueRef rhsInt = LLVMBuildPtrToInt(ctx->builder, rhsPtr, intptrTy, "ptr.diff.rhs");
    LLVMValueRef byteDiff = LLVMBuildSub(ctx->builder, lhsInt, rhsInt, "ptr.diff.bytes");

    /* Determine element size without relying on pointer element types (opaque-pointer safe). */
    uint64_t elemBytes = 1;
    if (lhsParsed) {
        LLVMTypeRef hinted = cg_element_type_hint_from_parsed(ctx, lhsParsed);
        uint64_t sz = 0;
        uint32_t al = 0;
        if (cg_size_align_for_type(ctx, lhsParsed, hinted, &sz, &al) && sz > 0) {
            elemBytes = sz;
        }
    }
    LLVMValueRef elementSize = LLVMConstInt(intptrTy, elemBytes, 0);
    return LLVMBuildSDiv(ctx->builder, byteDiff, elementSize, "ptr.diff");
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
            if (entry && entry->parsedType) {
                return entry->parsedType;
            }
            const SemanticModel* model = cg_context_get_semantic_model(ctx);
            if (model) {
                const Symbol* sym = semanticModelLookupGlobal(model, name);
                if (sym) {
                    return &sym->type;
                }
            }
            return NULL;
        }
        case AST_NUMBER_LITERAL:
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
            if (!node->expr.op || strcmp(node->expr.op, "!") != 0) {
                return cg_resolve_expression_type(ctx, node->expr.left);
            }
            return cg_builtin_signed_int_type();
        }
        case AST_POINTER_ACCESS:
        case AST_POINTER_DEREFERENCE:
        case AST_DOT_ACCESS:
        case AST_STRUCT_FIELD_ACCESS:
            return cg_resolve_expression_type(ctx, node->memberAccess.base);
        case AST_FUNCTION_CALL: {
            ASTNode* callee = node->functionCall.callee;
            if (callee && callee->type == AST_IDENTIFIER) {
                const SemanticModel* model = cg_context_get_semantic_model(ctx);
                if (model) {
                    const Symbol* sym = semanticModelLookupGlobal(model, callee->valueNode.value);
                    if (sym) {
                        return &sym->type;
                    }
                }
            }
            return NULL;
        }
        case AST_ARRAY_ACCESS: {
            const ParsedType* base = cg_resolve_expression_type(ctx, node->arrayAccess.array);
            static ParsedType cached;
            parsedTypeFree(&cached);
            cached.kind = TYPE_INVALID;
            if (!base) return NULL;
            if (parsedTypeIsDirectArray(base)) {
                cached = parsedTypeArrayElementType(base);
                return &cached;
            }
            ParsedType pointed = parsedTypePointerTargetType(base);
            if (pointed.kind != TYPE_INVALID) {
                cached = pointed;
                return &cached;
            }
            parsedTypeFree(&pointed);
            return NULL;
        }
        default:
            break;
    }
    return NULL;
}

bool cg_expression_is_unsigned(CodegenContext* ctx, ASTNode* node) {
    const ParsedType* type = cg_resolve_expression_type(ctx, node);
    if (type) {
        CGValueCategory cat = cg_classify_parsed_type(type);
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

    bool srcUnsigned = cg_should_treat_as_unsigned(fromParsed, sourceType);
    bool dstUnsigned = cg_should_treat_as_unsigned(toParsed, targetType);

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
