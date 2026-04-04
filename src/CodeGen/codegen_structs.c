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

static LLVMTypeRef cg_innermost_array_element_type(CodegenContext* ctx, const ParsedType* parsed) {
    ParsedType base = parsedTypeClone(parsed);
    while (parsedTypeIsDirectArray(&base)) {
        ParsedType next = parsedTypeArrayElementType(&base);
        parsedTypeFree(&base);
        base = next;
    }
    LLVMTypeRef elem = cg_type_from_parsed(ctx, &base);
    parsedTypeFree(&base);
    if (!elem || LLVMGetTypeKind(elem) == LLVMVoidTypeKind) {
        elem = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return elem;
}

static const CCTagFieldLayout* cg_lookup_field_layout(CodegenContext* ctx,
                                                      const ParsedType* aggregateParsed,
                                                      const char* fieldName) {
    if (!ctx || !aggregateParsed || !fieldName) return NULL;
    const ParsedType* resolved = aggregateParsed;
    size_t guard = 0;
    while (resolved &&
           resolved->tag != TAG_STRUCT &&
           resolved->tag != TAG_UNION &&
           resolved->kind == TYPE_NAMED &&
           resolved->userTypeName &&
           guard++ < 8) {
        const ParsedType* next = NULL;
        if (ctx->typeCache) {
            CGNamedLLVMType* info = cg_type_cache_get_typedef_info(ctx->typeCache, resolved->userTypeName);
            if (info && info->parsedType.kind != TYPE_INVALID) {
                next = &info->parsedType;
            }
        }
        if (!next) {
            const SemanticModel* model = cg_context_get_semantic_model(ctx);
            if (model) {
                const Symbol* sym = semanticModelLookupGlobal(model, resolved->userTypeName);
                if (sym && sym->kind == SYMBOL_TYPEDEF) {
                    next = &sym->type;
                }
            }
        }
        if (!next || next == resolved) {
            break;
        }
        resolved = next;
    }
    if (!resolved || (resolved->tag != TAG_STRUCT && resolved->tag != TAG_UNION)) return NULL;
    if (!resolved->userTypeName) return NULL;
    CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
    if (!cctx) return NULL;
    const CCTagFieldLayout* layouts = NULL;
    size_t count = 0;
    CCTagKind kind = (resolved->tag == TAG_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
    if (!cc_get_tag_field_layouts(cctx, kind, resolved->userTypeName, &layouts, &count)) {
        Scope* globalScope = semanticModelGetGlobalScope(ctx->semanticModel);
        layout_struct_union(cctx, globalScope, kind, resolved->userTypeName, NULL, NULL);
        cc_get_tag_field_layouts(cctx, kind, resolved->userTypeName, &layouts, &count);
    }
    for (size_t i = 0; i < count; ++i) {
        const CCTagFieldLayout* lay = &layouts[i];
        if (!lay->name) continue;
        if (strcmp(lay->name, fieldName) == 0) {
            return lay;
        }
    }
    return NULL;
}

static bool cg_parsed_is_flexible_array(const ParsedType* t) {
    if (!t) return false;
    if (!parsedTypeIsDirectArray(t)) return false;
    const TypeDerivation* last = NULL;
    size_t idx = 0;
    while (1) {
        const TypeDerivation* d = parsedTypeGetDerivation(t, idx);
        if (!d) break;
        last = d;
        idx++;
    }
    if (last && last->kind == TYPE_DERIVATION_ARRAY && last->as.array.isFlexible) {
        return true;
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

static const StructInfo* lookupStructInfo(CodegenContext* ctx, const char* name, LLVMTypeRef llvmType) {
    if (!ctx) return NULL;
    for (size_t i = 0; i < ctx->structInfoCount; ++i) {
        if (llvmType && ctx->structInfos[i].llvmType && ctx->structInfos[i].llvmType == llvmType) {
            return &ctx->structInfos[i];
        }
        if (name && ctx->structInfos[i].name && strcmp(ctx->structInfos[i].name, name) == 0) {
            return &ctx->structInfos[i];
        }
    }
    return NULL;
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

LLVMValueRef buildArrayElementPointer(CodegenContext* ctx,
                                      LLVMValueRef arrayPtr,
                                      LLVMValueRef index,
                                      const ParsedType* baseParsedHint,
                                      LLVMTypeRef aggregateTypeHint,
                                      LLVMTypeRef elementTypeHint,
                                      LLVMTypeRef* outElementType) {
    if (!ctx || !arrayPtr || !index) return NULL;
    const char* dbgRun = getenv("DEBUG_RUN");
    if (dbgRun) fprintf(stderr, "[CG] buildArrayElementPointer begin\n");

    LLVMTypeRef valueType = LLVMTypeOf(arrayPtr);
    if (!valueType) return NULL;

    LLVMValueRef basePtr = arrayPtr;
    LLVMTypeKind valueKind = LLVMGetTypeKind(valueType);
    const char* dbg = getenv("DEBUG_FLEX");
        if (dbg) {
            char* tstr = LLVMPrintTypeToString(valueType);
            fprintf(stderr, "[DBG] arrayPtr type=%s\n", tstr ? tstr : "<null>");
            if (tstr) LLVMDisposeMessage(tstr);
            LLVMTypeRef pt = NULL;
            if (LLVMGetTypeKind(valueType) == LLVMPointerTypeKind && !LLVMPointerTypeIsOpaque(valueType)) {
                pt = LLVMGetElementType(valueType);
            }
            if (pt) {
                char* pstr = LLVMPrintTypeToString(pt);
                fprintf(stderr, "[DBG] pointee=%s\n", pstr ? pstr : "<null>");
                if (pstr) LLVMDisposeMessage(pstr);
            }
    }

    /* Decay raw array values into an addressable pointer. */
    if (valueKind == LLVMArrayTypeKind) {
        LLVMValueRef tmp = LLVMBuildAlloca(ctx->builder, valueType, "array.decay.tmp");
        LLVMBuildStore(ctx->builder, arrayPtr, tmp);
        basePtr = tmp;
        valueType = LLVMTypeOf(basePtr);
        valueKind = LLVMGetTypeKind(valueType);
    }

    /* Wrap non-pointers into an alloca to take the address. */
    if (valueKind != LLVMPointerTypeKind) {
        LLVMValueRef tmp = LLVMBuildAlloca(ctx->builder, valueType, "array.ptr.wrap");
        LLVMBuildStore(ctx->builder, arrayPtr, tmp);
        basePtr = tmp;
        valueType = LLVMTypeOf(basePtr);
        valueKind = LLVMGetTypeKind(valueType);
    }
    if (valueKind != LLVMPointerTypeKind) {
        return NULL;
    }

    /* Resolve element type. */
    bool explicitElementHint = elementTypeHint != NULL;
    LLVMTypeRef elementType = elementTypeHint;
    if (!elementType) {
        elementType = cg_element_type_from_pointer(ctx, baseParsedHint, valueType);
    }
    LLVMTypeRef pointee = NULL;
    if (!LLVMPointerTypeIsOpaque(valueType)) {
        pointee = LLVMGetElementType(valueType);
    }
    if (!pointee && aggregateTypeHint) {
        pointee = aggregateTypeHint;
    }
    if (!pointee && elementType) {
        /* Opaque pointer with an element hint; accept the hint. */
        pointee = NULL;
    }
    if (pointee && LLVMGetTypeKind(pointee) == LLVMFunctionTypeKind) {
        pointee = LLVMPointerType(pointee, 0);
    }
    if (elementType && LLVMGetTypeKind(elementType) == LLVMFunctionTypeKind) {
        elementType = LLVMPointerType(elementType, 0);
    }
    if (pointee && LLVMGetTypeKind(pointee) == LLVMArrayTypeKind) {
        LLVMTypeRef pointeeElement = LLVMGetElementType(pointee);
        if (pointeeElement && (!elementType || !explicitElementHint)) {
            /*
             * When parsed hints are unavailable we may have fallen back to i8 from
             * an opaque pointer. Prefer the concrete pointee array element type.
             */
            elementType = pointeeElement;
        }
        if (!elementType) {
            elementType = LLVMGetElementType(pointee);
        }
    }
    if (elementType && LLVMGetTypeKind(elementType) == LLVMArrayTypeKind && LLVMGetArrayLength(elementType) == 0) {
        elementType = LLVMGetElementType(elementType);
    }
    if (elementType && LLVMGetTypeKind(elementType) == LLVMFunctionTypeKind) {
        elementType = LLVMPointerType(elementType, 0);
    }
    if (!elementType || LLVMGetTypeKind(elementType) == LLVMVoidTypeKind ||
        LLVMGetTypeKind(elementType) == LLVMHalfTypeKind) {
        /* Default to a 32-bit element for unknown/opaque pointers to avoid bogus half-element GEPs. */
        elementType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    if (pointee &&
        (LLVMGetTypeKind(pointee) == LLVMHalfTypeKind || LLVMGetTypeKind(pointee) == LLVMFloatTypeKind) &&
        (!elementType || LLVMGetTypeKind(elementType) == LLVMVoidTypeKind)) {
        elementType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    /* Keep aggregate element hints for opaque pointers; downgrading to i32 loses stride semantics. */
    if (LLVMGetTypeKind(valueType) == LLVMPointerTypeKind &&
        LLVMGetElementType(valueType) == NULL &&
        elementType &&
        LLVMGetTypeKind(elementType) == LLVMHalfTypeKind) {
        elementType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    LLVMTypeRef i64Type = LLVMInt64TypeInContext(ctx->llvmContext);
    LLVMValueRef idx64 = index;
    LLVMTypeRef idxType = LLVMTypeOf(idx64);
    if (!idxType || LLVMGetTypeKind(idxType) != LLVMIntegerTypeKind || LLVMGetIntTypeWidth(idxType) < 64) {
        idx64 = LLVMBuildSExt(ctx->builder, idx64, i64Type, "array.idx64");
    }

    if (baseParsedHint && cg_parsed_type_has_pointer_layer(baseParsedHint)) {
        ParsedType pointed = parsedTypePointerTargetType(baseParsedHint);
        if (pointed.kind != TYPE_INVALID &&
            parsedTypeIsDirectArray(&pointed) &&
            parsedTypeHasVLA(&pointed)) {
            LLVMValueRef stepped = cg_build_pointer_offset(ctx, basePtr, idx64, baseParsedHint, NULL, false);
            LLVMTypeRef elem = cg_innermost_array_element_type(ctx, &pointed);
            parsedTypeFree(&pointed);
            if (!stepped) {
                return NULL;
            }
            if (outElementType) {
                *outElementType = elem;
            }
            return stepped;
        }
        parsedTypeFree(&pointed);
    }

    /* Prefer typed GEPs to avoid opaque-pointer crashes. */
    LLVMValueRef indices[2];
    unsigned indexCount = 0;
    if (pointee && LLVMGetTypeKind(pointee) == LLVMArrayTypeKind) {
        if (dbg) {
            char* bstr = LLVMPrintTypeToString(LLVMTypeOf(basePtr));
            char* pstr = LLVMPrintTypeToString(pointee);
            fprintf(stderr, "[DBG] array branch baseTy=%s pointee=%s\n", bstr ? bstr : "<null>", pstr ? pstr : "<null>");
            if (bstr) LLVMDisposeMessage(bstr);
            if (pstr) LLVMDisposeMessage(pstr);
        }
        LLVMValueRef zero = LLVMConstInt(i64Type, 0, 0);
        indices[0] = zero;
        indices[1] = idx64;
        indexCount = 2;
        LLVMTypeRef baseElem = NULL;
        if (LLVMGetTypeKind(LLVMTypeOf(basePtr)) == LLVMPointerTypeKind) {
            LLVMTypeRef basePtrTy = LLVMTypeOf(basePtr);
            if (!LLVMPointerTypeIsOpaque(basePtrTy)) {
                baseElem = LLVMGetElementType(basePtrTy);
            }
        }
    if (baseElem && baseElem != pointee) {
        return NULL;
    }
    LLVMValueRef gep = LLVMBuildGEP2(ctx->builder, pointee, basePtr, indices, indexCount, "arrayElemPtr");
    if (outElementType) *outElementType = elementType;
    if (dbgRun) fprintf(stderr, "[CG] buildArrayElementPointer array branch end\n");
    return gep;
    }

    indices[0] = idx64;
    indexCount = 1;
    LLVMValueRef baseCasted = basePtr;
    if (dbg) {
        char* bstr = LLVMPrintTypeToString(LLVMTypeOf(basePtr));
        char* estr = LLVMPrintTypeToString(elementType);
        fprintf(stderr, "[DBG] elem branch baseTy=%s elem=%s\n", bstr ? bstr : "<null>", estr ? estr : "<null>");
        if (bstr) LLVMDisposeMessage(bstr);
        if (estr) LLVMDisposeMessage(estr);
    }
    LLVMTypeRef baseElem = NULL;
    if (LLVMGetTypeKind(LLVMTypeOf(baseCasted)) == LLVMPointerTypeKind) {
        LLVMTypeRef basePtrTy = LLVMTypeOf(baseCasted);
        if (!LLVMPointerTypeIsOpaque(basePtrTy)) {
            baseElem = LLVMGetElementType(basePtrTy);
        }
    }
    if (!baseElem && !elementType) {
        fprintf(stderr, "Error: opaque pointer array base without element type\n");
        return NULL;
    }
    LLVMTypeRef geptype = elementType ? elementType : baseElem;
    if (geptype && LLVMGetTypeKind(geptype) == LLVMHalfTypeKind) {
        geptype = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    LLVMValueRef gep = LLVMBuildGEP2(ctx->builder, geptype, baseCasted, indices, indexCount, "arrayElemPtr");
    if (outElementType) {
        *outElementType = elementType ? elementType : geptype;
    }
    if (dbgRun) fprintf(stderr, "[CG] buildArrayElementPointer element branch end\n");
    return gep;
}

static CGStructLLVMInfo* findStructInfoForAggregate(CodegenContext* ctx,
                                                    LLVMTypeRef aggregateType,
                                                    const char* structNameHint,
                                                    const ParsedType* parsedHint) {
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (!cache) {
        return NULL;
    }
    if (parsedHint && parsedHint->inlineStructOrUnionDef) {
        CGStructLLVMInfo* info =
            cg_type_cache_get_struct_by_definition(cache, parsedHint->inlineStructOrUnionDef);
        if (info) {
            return info;
        }
    }
    if (aggregateType) {
        CGStructLLVMInfo* info = cg_type_cache_find_struct_by_llvm(cache, aggregateType);
        if (info) {
            return info;
        }
    }
    if (structNameHint && structNameHint[0]) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(cache, structNameHint);
        if (info) {
            return info;
        }
    }
    if (parsedHint && parsedHint->userTypeName) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(cache, parsedHint->userTypeName);
        if (info) {
            return info;
        }
    }
    return NULL;
}

LLVMValueRef buildStructFieldPointer(CodegenContext* ctx,
                                     LLVMValueRef basePtr,
                                     LLVMTypeRef aggregateTypeHint,
                                     const char* structName,
                                     const char* fieldName,
                                     const ParsedType* structParsedHint,
                                     LLVMTypeRef* outFieldType,
                                     const ParsedType** outFieldParsedType) {
    if (!ctx || !basePtr || !fieldName) {
        return NULL;
    }

    LLVMTypeRef ptrType = LLVMTypeOf(basePtr);
    if (LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
        return NULL;
    }

    LLVMTypeRef aggregateType = aggregateTypeHint;
    if (!aggregateType) {
        aggregateType = cg_dereference_ptr_type(ctx, ptrType, "struct field access");
    }
    ParsedType pointedScratch;
    bool havePointedScratch = false;
    const ParsedType* structHint = structParsedHint;
    if (structHint && (structHint->pointerDepth > 0 || structHint->isFunctionPointer)) {
        pointedScratch = parsedTypePointerTargetType(structHint);
        if (pointedScratch.kind != TYPE_INVALID) {
            structHint = &pointedScratch;
            havePointedScratch = true;
        }
    }
    if ((!aggregateType || LLVMGetTypeKind(aggregateType) != LLVMStructTypeKind) && structHint) {
        aggregateType = cg_type_from_parsed(ctx, structHint);
    }
    if (!aggregateType && structHint && structHint->userTypeName) {
        aggregateType = ensureStructLLVMTypeByName(ctx, structHint->userTypeName, structHint->tag == TAG_UNION);
    }
    if ((!aggregateType || LLVMGetTypeKind(aggregateType) == LLVMVoidTypeKind) && LLVMIsAAllocaInst(basePtr)) {
        LLVMTypeRef allocated = LLVMGetAllocatedType(basePtr);
        if (allocated) {
            aggregateType = allocated;
        }
    }
    if (!aggregateType || LLVMGetTypeKind(aggregateType) != LLVMStructTypeKind) {
        if (havePointedScratch) {
            parsedTypeFree(&pointedScratch);
        }
        return NULL;
    }

    const char* structNameHint = structName ? structName : NULL;
    if (!structNameHint && structHint && structHint->userTypeName) {
        structNameHint = structHint->userTypeName;
    }
    if (!structNameHint) {
        structNameHint = LLVMGetStructName(aggregateType);
    }
    unsigned fieldIndex = 0;
    LLVMTypeRef fieldLLVMType = NULL;
    bool found = false;

    bool isUnion = false;
    CGStructLLVMInfo* semanticInfo =
        findStructInfoForAggregate(ctx, aggregateType, structNameHint, structParsedHint);
    if (semanticInfo) {
        isUnion = semanticInfo->isUnion;
        if (!semanticInfo->llvmType && structNameHint) {
            semanticInfo->llvmType = ensureStructLLVMTypeByName(ctx, structNameHint, isUnion);
        }
        for (size_t i = 0; i < semanticInfo->fieldCount; ++i) {
            if (semanticInfo->fields[i].name && strcmp(semanticInfo->fields[i].name, fieldName) == 0) {
                fieldIndex = semanticInfo->isUnion ? 0 : semanticInfo->fields[i].index;
                fieldLLVMType = cg_type_from_parsed(ctx, &semanticInfo->fields[i].parsedType);
                if (outFieldParsedType) {
                    *outFieldParsedType = &semanticInfo->fields[i].parsedType;
                }
                found = true;
                break;
            }
        }
    }

    if (!found) {
        const StructInfo* legacy = lookupStructInfo(ctx, structNameHint, aggregateType);
        if (legacy) {
            for (size_t i = 0; i < legacy->fieldCount; ++i) {
                if (legacy->fields[i].name && strcmp(legacy->fields[i].name, fieldName) == 0) {
                    fieldIndex = legacy->isUnion ? 0 : legacy->fields[i].index;
                    fieldLLVMType = legacy->fields[i].type;
                    found = true;
                    break;
                }
            }
        }
    }

    if (!found && structParsedHint && structParsedHint->inlineStructOrUnionDef) {
        ASTNode* def = structParsedHint->inlineStructOrUnionDef;
        if (def->type == AST_STRUCT_DEFINITION || def->type == AST_UNION_DEFINITION) {
            bool inlineUnion = (def->type == AST_UNION_DEFINITION);
            unsigned runningIndex = 0;
            for (size_t i = 0; i < def->structDef.fieldCount && !found; ++i) {
                ASTNode* fieldDecl = def->structDef.fields ? def->structDef.fields[i] : NULL;
                if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) {
                    continue;
                }
                for (size_t v = 0; v < fieldDecl->varDecl.varCount; ++v) {
                    ASTNode* fieldNameNode = fieldDecl->varDecl.varNames ? fieldDecl->varDecl.varNames[v] : NULL;
                    const char* candidate = (fieldNameNode && fieldNameNode->type == AST_IDENTIFIER)
                        ? fieldNameNode->valueNode.value
                        : NULL;
                    if (candidate && strcmp(candidate, fieldName) == 0) {
                        fieldIndex = inlineUnion ? 0 : runningIndex;
                        const ParsedType* parsedField = astVarDeclTypeAt(fieldDecl, v);
                        fieldLLVMType = cg_type_from_parsed(ctx, parsedField);
                        if (outFieldParsedType) {
                            *outFieldParsedType = parsedField;
                        }
                        found = true;
                        break;
                    }
                    if (!inlineUnion) {
                        runningIndex++;
                    }
                }
            }
        }
    }

    if (!found) {
        CG_DEBUG("struct lookup failed for %s.%s (structInfos=%zu)",
                 structNameHint ? structNameHint : "<anon>",
                 fieldName,
                 ctx->structInfoCount);
        return NULL;
    }

    if (!fieldLLVMType || LLVMGetTypeKind(fieldLLVMType) == LLVMVoidTypeKind) {
        fieldLLVMType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    bool isFlexArrayField = false;
    if (outFieldParsedType && *outFieldParsedType) {
        isFlexArrayField = cg_parsed_is_flexible_array(*outFieldParsedType);
    }
    if (!isFlexArrayField && fieldLLVMType && LLVMGetTypeKind(fieldLLVMType) == LLVMArrayTypeKind &&
        LLVMGetArrayLength(fieldLLVMType) == 0) {
        isFlexArrayField = true;
    }

    const CCTagFieldLayout* lay = cg_lookup_field_layout(ctx, structHint, fieldName);
    if (lay && !lay->isBitfield) {
        LLVMTypeRef pointeeTy = fieldLLVMType;
        if (!pointeeTy || LLVMGetTypeKind(pointeeTy) == LLVMVoidTypeKind) {
            pointeeTy = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        if (isFlexArrayField && LLVMGetTypeKind(pointeeTy) == LLVMArrayTypeKind) {
            LLVMTypeRef elemTy = LLVMGetElementType(pointeeTy);
            if (!elemTy || LLVMGetTypeKind(elemTy) == LLVMVoidTypeKind) {
                elemTy = LLVMInt8TypeInContext(ctx->llvmContext);
            }
            pointeeTy = elemTy;
        }

        LLVMTypeRef i8Ty = LLVMInt8TypeInContext(ctx->llvmContext);
        LLVMValueRef baseI8 = LLVMBuildBitCast(ctx->builder, basePtr, LLVMPointerType(i8Ty, 0), "field.base.i8");
        LLVMValueRef offsetVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), lay->byteOffset, 0);
        LLVMValueRef ptrI8 = LLVMBuildGEP2(ctx->builder, i8Ty, baseI8, &offsetVal, 1, "field.byte.gep");
        LLVMValueRef ptr = LLVMBuildBitCast(ctx->builder,
                                            ptrI8,
                                            LLVMPointerType(pointeeTy, 0),
                                            "field.byte.ptr");
        if (outFieldType) {
            *outFieldType = pointeeTy;
        }
        if (havePointedScratch) {
            parsedTypeFree(&pointedScratch);
        }
        return ptr;
    }

    LLVMValueRef ptr = LLVMBuildStructGEP2(ctx->builder, aggregateType, basePtr, fieldIndex, "fieldPtr");
    if (!ptr) {
        if (havePointedScratch) parsedTypeFree(&pointedScratch);
        return NULL;
    }
    if (isFlexArrayField && fieldLLVMType && LLVMGetTypeKind(fieldLLVMType) == LLVMArrayTypeKind) {
        LLVMTypeRef elemTy = LLVMGetElementType(fieldLLVMType);
        if (!elemTy || LLVMGetTypeKind(elemTy) == LLVMVoidTypeKind) {
            elemTy = LLVMInt8TypeInContext(ctx->llvmContext);
        }
        /* Re-materialize the flexible member as a byte-based pointer to its first element
           rather than a pointer to a zero-length array, so downstream GEPs/bitcasts stay
           well-typed and avoid ASan-poisoned casts. */
        LLVMTypeRef i8Ty = LLVMInt8TypeInContext(ctx->llvmContext);
        LLVMValueRef baseI8 = LLVMBuildBitCast(ctx->builder, basePtr, LLVMPointerType(i8Ty, 0), "flex.base");

        uint64_t byteOffset = 0;
        const CCTagFieldLayout* lay = cg_lookup_field_layout(ctx, structHint, fieldName);
        if (lay) {
            byteOffset = lay->byteOffset;
        }
        LLVMValueRef offsetVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), byteOffset, 0);
        LLVMValueRef ptrI8 = LLVMBuildGEP2(ctx->builder, i8Ty, baseI8, &offsetVal, 1, "flex.byteptr");
        LLVMValueRef elemPtr = LLVMBuildBitCast(ctx->builder, ptrI8, LLVMPointerType(elemTy, 0), "flex.ptr");

        if (outFieldType) {
            *outFieldType = elemTy;
        }
        /* Keep parsed type as-is (flexible array) so downstream size/element
           queries can use the array metadata. */
        if (havePointedScratch) {
            parsedTypeFree(&pointedScratch);
        }
        return elemPtr;
    }

    if (outFieldType) {
        *outFieldType = fieldLLVMType;
    }

    if (havePointedScratch) {
        parsedTypeFree(&pointedScratch);
    }

    return ptr;
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
            LLVMTypeRef elementHint = cg_element_type_hint_from_parsed(ctx, baseParsed);
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
                } else if (elemParsed && cg_parsed_type_has_pointer_layer(elemParsed)) {
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
                    LLVMValueRef tmpAlloca = LLVMBuildAlloca(ctx->builder, LLVMTypeOf(baseVal), "dot_tmp_lhs");
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
            const ParsedType* structHint = (pointed.kind != TYPE_INVALID) ? &pointed : baseParsed;
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
                if (structHint == &pointed) {
                    parsedTypeFree(&pointed);
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
                if (structHint == &pointed) parsedTypeFree(&pointed);
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
            if (structHint == &pointed) {
                parsedTypeFree(&pointed);
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
