#include "codegen_private.h"

#include "codegen_types.h"
#include "Compiler/compiler_context.h"
#include "Syntax/layout.h"
#include "Syntax/literal_utils.h"

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
        /* Opaque pointer: let caller decide; treat as i8 for resilience. */
        return LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx));
    }
    return elem;
}

LLVMTypeRef cg_element_type_from_pointer_parsed(CodegenContext* ctx, const ParsedType* parsed) {
    if (!parsed || parsed->pointerDepth <= 0) {
        return NULL;
    }
    ParsedType base = parsedTypeClone(parsed);
    if (base.pointerDepth > 0) {
        base.pointerDepth -= 1;
    }
    /* Remove the last pointer derivation if present to keep derivations consistent. */
    for (ssize_t i = (ssize_t)base.derivationCount - 1; i >= 0; --i) {
        if (base.derivations[i].kind == TYPE_DERIVATION_POINTER) {
            for (size_t j = (size_t)i; j + 1 < base.derivationCount; ++j) {
                base.derivations[j] = base.derivations[j + 1];
            }
            base.derivationCount -= 1;
            break;
        }
    }
    LLVMTypeRef elem = cg_type_from_parsed(ctx, &base);
    parsedTypeFree(&base);
    return elem;
}

bool cg_is_volatile_object(const ParsedType* parsed) {
    if (!parsed) {
        return false;
    }
    if (parsed->pointerDepth > 0) {
        for (size_t i = 0; i < parsed->derivationCount; ++i) {
            const TypeDerivation* deriv = parsedTypeGetDerivation(parsed, i);
            if (!deriv) {
                break;
            }
            if (deriv->kind == TYPE_DERIVATION_POINTER) {
                return deriv->as.pointer.isVolatile;
            }
            if (deriv->kind == TYPE_DERIVATION_ARRAY) {
                return deriv->as.array.qualVolatile;
            }
        }
        return false;
    }
    return parsed->isVolatile;
}

LLVMValueRef cg_build_load(CodegenContext* ctx,
                           LLVMTypeRef type,
                           LLVMValueRef ptr,
                           const char* name,
                           const ParsedType* parsed) {
    LLVMValueRef load = LLVMBuildLoad2(ctx->builder, type, ptr, name ? name : "");
    if (cg_is_volatile_object(parsed)) {
        LLVMSetVolatile(load, 1);
    }
    return load;
}

LLVMValueRef cg_build_store(CodegenContext* ctx,
                            LLVMValueRef value,
                            LLVMValueRef ptr,
                            const ParsedType* parsed) {
    LLVMValueRef store = LLVMBuildStore(ctx->builder, value, ptr);
    if (cg_is_volatile_object(parsed)) {
        LLVMSetVolatile(store, 1);
    }
    return store;
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
    for (const char* p = text; *p; ++p) {
        if (*p == '.') {
            return true;
        }
        if (!isHex && (*p == 'e' || *p == 'E')) {
            return true;
        }
        if (isHex && (*p == 'p' || *p == 'P')) {
            return true;
        }
    }
    size_t len = strlen(text);
    if (len > 0) {
        char last = text[len - 1];
        if (last == 'f' || last == 'F' || last == 'l' || last == 'L' ||
            last == 'i' || last == 'I' || last == 'j' || last == 'J') {
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

static bool cg_parsed_type_is_pointer(const ParsedType* type) {
    if (!type) return false;
    if (type->isFunctionPointer) return true;
    return type->pointerDepth > 0;
}

static const ParsedType* cg_resolve_typedef_parsed(CodegenContext* ctx, const ParsedType* type) {
    if (!ctx || !type || type->kind != TYPE_NAMED || !type->userTypeName) {
        return type;
    }
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (!cache) return type;
    CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, type->userTypeName);
    if (info) {
        return &info->parsedType;
    }
    return type;
}

static bool cg_parsed_type_is_complex(const ParsedType* type) {
    return type && (type->isComplex || type->isImaginary);
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
        if (getenv("DEBUG_PTR_ARITH")) {
            fprintf(stderr, "Error: pointer arithmetic requires pointer base\n");
        }
        return NULL;
    }
    if (getenv("DEBUG_PTR_ARITH") && pointerParsed) {
        fprintf(stderr, "[ptr-arith] pointerDepth=%d derivations=", pointerParsed->pointerDepth);
        for (size_t i = 0; i < pointerParsed->derivationCount; ++i) {
            const TypeDerivation* deriv = &pointerParsed->derivations[i];
            const char* kind = "?";
            if (deriv->kind == TYPE_DERIVATION_ARRAY) kind = "array";
            else if (deriv->kind == TYPE_DERIVATION_POINTER) kind = "ptr";
            else if (deriv->kind == TYPE_DERIVATION_FUNCTION) kind = "func";
            fprintf(stderr, "%s%s", (i ? "," : ""), kind);
        }
        fprintf(stderr, "\n");
    }
    /* If we got a pointer-to-array (e.g., the name of an array variable), decay
       to the element pointer before computing offsets. */
    const ParsedType* elemParsed = pointerParsed;
    if (pointerParsed && parsedTypeIsDirectArray(pointerParsed)) {
        static ParsedType tmpElem;
        parsedTypeFree(&tmpElem);
        tmpElem = parsedTypeArrayElementType(pointerParsed);
        elemParsed = &tmpElem;

        LLVMTypeRef arrTy = cg_dereference_ptr_type(ctx, ptrType, "pointer arithmetic decay");
        if (arrTy && LLVMGetTypeKind(arrTy) == LLVMArrayTypeKind) {
            LLVMValueRef zero32 = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
            LLVMValueRef idxs[2] = { zero32, zero32 };
            basePtr = LLVMBuildGEP2(ctx->builder, arrTy, basePtr, idxs, 2, "ptr.arith.decay");
            ptrType = LLVMTypeOf(basePtr);
        }
    }

    LLVMValueRef index = offsetValue;
    LLVMTypeRef offsetType = LLVMTypeOf(offsetValue);
    if (offsetType && LLVMGetTypeKind(offsetType) == LLVMIntegerTypeKind) {
        LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
        if (offsetType != intptrTy) {
            index = LLVMBuildIntCast2(ctx->builder, offsetValue, intptrTy, 0, "ptr.idx.cast");
        }
    } else if (LLVMGetTypeKind(offsetType) == LLVMPointerTypeKind) {
        if (getenv("DEBUG_PTR_ARITH")) {
            fprintf(stderr, "Error: pointer arithmetic with pointer offset unsupported\n");
        }
        return NULL;
    } else {
        if (getenv("DEBUG_PTR_ARITH")) {
            fprintf(stderr, "Error: pointer arithmetic offset must be integer\n");
        }
        return NULL;
    }

    if (isSubtract) {
        index = LLVMBuildNeg(ctx->builder, index, "ptr.idx.neg");
    }

    /* Determine element size; prefer semantic info, fall back to LLVM sizes, then 1 byte. */
    LLVMTypeRef elementType = cg_element_type_from_pointer(ctx, elemParsed, ptrType);
    ParsedType targetParsed = parsedTypePointerTargetType(elemParsed);
    const ParsedType* sizeParsed = elemParsed;
    if (targetParsed.kind != TYPE_INVALID) {
        sizeParsed = &targetParsed;
    }
    uint64_t elemSize = 0;
    uint32_t elemAlign = 0;
    if (!cg_size_align_for_type(ctx, sizeParsed, elementType, &elemSize, &elemAlign) || elemSize == 0) {
        LLVMTargetDataRef td = ctx && ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
        if (td && elementType) {
            elemSize = LLVMABISizeOfType(td, elementType);
        }
    }
    if (targetParsed.kind != TYPE_INVALID) {
        parsedTypeFree(&targetParsed);
    }
    if (elemSize == 0) elemSize = 1;

    LLVMTypeRef i8Type = LLVMInt8TypeInContext(ctx->llvmContext);
    LLVMTypeRef bytePtrTy = LLVMPointerType(i8Type, 0);
    LLVMValueRef baseI8 = LLVMBuildPointerCast(ctx->builder, basePtr, bytePtrTy, "ptr.byte.base");
    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    LLVMValueRef elemSizeVal = LLVMConstInt(intptrTy, elemSize, 0);
    LLVMValueRef byteOffset = LLVMBuildMul(ctx->builder, index, elemSizeVal, "ptr.byte.offset");
    LLVMValueRef gep = LLVMBuildGEP2(ctx->builder, i8Type, baseI8, &byteOffset, 1, "ptr.arith");
    if (!gep) return NULL;

    /* Return byte-addressed pointer; caller can cast if a concrete element type is required. */
    return gep;
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

    LLVMTypeRef i8Type = LLVMInt8TypeInContext(ctx->llvmContext);
    LLVMTypeRef bytePtrTy = LLVMPointerType(i8Type, 0);
    LLVMValueRef lhsByte = LLVMBuildPointerCast(ctx->builder, lhsPtr, bytePtrTy, "ptr.diff.lhs.byte");
    LLVMValueRef rhsByte = LLVMBuildPointerCast(ctx->builder, rhsPtr, bytePtrTy, "ptr.diff.rhs.byte");

    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    LLVMValueRef lhsInt = LLVMBuildPtrToInt(ctx->builder, lhsByte, intptrTy, "ptr.diff.lhs");
    LLVMValueRef rhsInt = LLVMBuildPtrToInt(ctx->builder, rhsByte, intptrTy, "ptr.diff.rhs");
    LLVMValueRef byteDiff = LLVMBuildSub(ctx->builder, lhsInt, rhsInt, "ptr.diff.bytes");

    /* Determine element size without relying on pointer element types (opaque-pointer safe). */
    uint64_t elemBytes = 0;
    ParsedType elemParsed = {0};
    bool haveElemParsed = false;
    LLVMTypeRef elemHint = cg_element_type_hint_from_parsed(ctx, lhsParsed);
    if (lhsParsed) {
        if (parsedTypeIsDirectArray(lhsParsed)) {
            elemParsed = parsedTypeArrayElementType(lhsParsed);
            haveElemParsed = true;
        } else {
            elemParsed = parsedTypePointerTargetType(lhsParsed);
            haveElemParsed = elemParsed.kind != TYPE_INVALID;
            if (!haveElemParsed) {
                parsedTypeFree(&elemParsed);
            }
        }
    }
    if (haveElemParsed) {
        uint32_t al = 0;
        cg_size_align_for_type(ctx, &elemParsed, elemHint, &elemBytes, &al);
        parsedTypeFree(&elemParsed);
    }
    if (elemBytes == 0 && elemHint) {
        LLVMTargetDataRef td = ctx && ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
        if (td) {
            elemBytes = LLVMABISizeOfType(td, elemHint);
        }
    }
    if (elemBytes == 0) {
        CG_ERROR("Cannot compute element size for pointer difference");
        return NULL;
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
            const ParsedType* base = cg_resolve_expression_type(ctx, node->memberAccess.base);
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
                if (info && node->memberAccess.field) {
                    for (size_t i = 0; i < info->fieldCount; ++i) {
                        if (info->fields[i].name &&
                            strcmp(info->fields[i].name, node->memberAccess.field) == 0) {
                            parsedTypeFree(&baseCopy);
                            return &info->fields[i].parsedType;
                        }
                    }
                }
                if (ctx && ctx->semanticModel && node->memberAccess.field) {
                    CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                    CCTagKind tagKind = (structParsed->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                    ASTNode* def = cctx ? cc_tag_definition(cctx, tagKind, structParsed->userTypeName) : NULL;
                    if (def && (def->type == AST_STRUCT_DEFINITION || def->type == AST_UNION_DEFINITION)) {
                        for (size_t f = 0; f < def->structDef.fieldCount; ++f) {
                            ASTNode* fieldDecl = def->structDef.fields ? def->structDef.fields[f] : NULL;
                            if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
                            for (size_t v = 0; v < fieldDecl->varDecl.varCount; ++v) {
                                ASTNode* nameNode = fieldDecl->varDecl.varNames[v];
                                const char* fieldName = (nameNode && nameNode->type == AST_IDENTIFIER)
                                    ? nameNode->valueNode.value
                                    : NULL;
                                if (fieldName && strcmp(fieldName, node->memberAccess.field) == 0) {
                                    parsedTypeFree(&baseCopy);
                                    return astVarDeclTypeAt(fieldDecl, v);
                                }
                            }
                        }
                    }
                }
            }
            parsedTypeFree(&baseCopy);
            return base;
        }
        case AST_POINTER_DEREFERENCE:
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
