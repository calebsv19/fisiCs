// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"
#include "Compiler/compiler_context.h"
#include "Syntax/layout.h"

#include "codegen_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

const CCTagFieldLayout* cg_lookup_field_layout(CodegenContext* ctx,
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

static bool cg_parsed_is_record_like(const ParsedType* parsed) {
    if (!parsed) return false;
    if (parsed->tag == TAG_STRUCT || parsed->tag == TAG_UNION) {
        return true;
    }
    if (parsed->inlineStructOrUnionDef) {
        return true;
    }
    return false;
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

static uint64_t cg_field_offset_bytes(CodegenContext* ctx,
                                      LLVMTypeRef aggregateType,
                                      bool isUnion,
                                      unsigned fieldIndex) {
    if (!ctx || !aggregateType || isUnion) {
        return 0;
    }
    LLVMModuleRef module = cg_context_get_module(ctx);
    LLVMTargetDataRef td = module ? LLVMGetModuleDataLayout(module) : NULL;
    if (!td) {
        return 0;
    }
    return LLVMOffsetOfElement(td, aggregateType, fieldIndex);
}

static bool cg_lookup_nested_field_in_legacy(CodegenContext* ctx,
                                             const StructInfo* info,
                                             const char* fieldName,
                                             uint64_t baseOffset,
                                             LLVMTypeRef* outFieldType,
                                             const ParsedType** outFieldParsedType,
                                             uint64_t* outByteOffset,
                                             unsigned depth) {
    if (!ctx || !info || !fieldName || depth > 16) {
        return false;
    }

    for (size_t i = 0; i < info->fieldCount; ++i) {
        const StructFieldInfo* field = &info->fields[i];
        uint64_t fieldOffset = baseOffset + cg_field_offset_bytes(ctx, info->llvmType, info->isUnion, field->index);
        if (field->name && strcmp(field->name, fieldName) == 0) {
            if (outFieldType) {
                *outFieldType = field->type;
            }
            if (outFieldParsedType) {
                *outFieldParsedType = &field->parsedType;
            }
            if (outByteOffset) {
                *outByteOffset = fieldOffset;
            }
            return true;
        }

        if (field->name == NULL && cg_parsed_is_record_like(&field->parsedType)) {
            const StructInfo* nested = lookupStructInfo(ctx, NULL, field->type);
            if (nested && cg_lookup_nested_field_in_legacy(ctx,
                                                           nested,
                                                           fieldName,
                                                           fieldOffset,
                                                           outFieldType,
                                                           outFieldParsedType,
                                                           outByteOffset,
                                                           depth + 1)) {
                return true;
            }
        }
    }

    return false;
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

    if (valueKind == LLVMArrayTypeKind) {
        LLVMValueRef tmp = cg_build_entry_alloca(ctx, valueType, "array.decay.tmp");
        LLVMBuildStore(ctx->builder, arrayPtr, tmp);
        basePtr = tmp;
        valueType = LLVMTypeOf(basePtr);
        valueKind = LLVMGetTypeKind(valueType);
    }

    if (valueKind != LLVMPointerTypeKind) {
        LLVMValueRef tmp = cg_build_entry_alloca(ctx, valueType, "array.ptr.wrap");
        LLVMBuildStore(ctx->builder, arrayPtr, tmp);
        basePtr = tmp;
        valueType = LLVMTypeOf(basePtr);
        valueKind = LLVMGetTypeKind(valueType);
    }
    if (valueKind != LLVMPointerTypeKind) {
        return NULL;
    }

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
        elementType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    if (pointee &&
        (LLVMGetTypeKind(pointee) == LLVMHalfTypeKind || LLVMGetTypeKind(pointee) == LLVMFloatTypeKind) &&
        (!elementType || LLVMGetTypeKind(elementType) == LLVMVoidTypeKind)) {
        elementType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
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
    const ParsedType* resolvedHint = parsedHint;
    size_t aliasGuard = 0;
    while (resolvedHint &&
           resolvedHint->kind == TYPE_NAMED &&
           resolvedHint->userTypeName &&
           aliasGuard++ < 16) {
        const ParsedType* next = NULL;
        if (ctx && ctx->typeCache) {
            CGNamedLLVMType* info =
                cg_type_cache_get_typedef_info(ctx->typeCache, resolvedHint->userTypeName);
            if (info && info->parsedType.kind != TYPE_INVALID) {
                next = &info->parsedType;
            }
        }
        if (!next && ctx) {
            const SemanticModel* model = cg_context_get_semantic_model(ctx);
            if (model) {
                const Symbol* sym = semanticModelLookupGlobal(model, resolvedHint->userTypeName);
                if (sym && sym->kind == SYMBOL_TYPEDEF) {
                    next = &sym->type;
                }
            }
        }
        if (!next || next == resolvedHint) {
            break;
        }
        resolvedHint = next;
    }

    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (!cache) {
        return NULL;
    }
    if (resolvedHint && resolvedHint->inlineStructOrUnionDef) {
        CGStructLLVMInfo* info =
            cg_type_cache_get_struct_by_definition(cache, resolvedHint->inlineStructOrUnionDef);
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
    if (resolvedHint && resolvedHint->userTypeName) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(cache, resolvedHint->userTypeName);
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
    bool useByteOffsetOnly = false;
    uint64_t resolvedByteOffset = 0;

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
            if (!found &&
                cg_lookup_nested_field_in_legacy(ctx,
                                                 legacy,
                                                 fieldName,
                                                 0,
                                                 &fieldLLVMType,
                                                 outFieldParsedType,
                                                 &resolvedByteOffset,
                                                 0)) {
                found = true;
                useByteOffsetOnly = true;
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

    LLVMValueRef ptr = NULL;
    if (!useByteOffsetOnly) {
        ptr = LLVMBuildStructGEP2(ctx->builder, aggregateType, basePtr, fieldIndex, "fieldPtr");
    }
    if ((useByteOffsetOnly || !ptr) && ((lay && !lay->isBitfield) || useByteOffsetOnly)) {
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
        uint64_t byteOffset = useByteOffsetOnly ? resolvedByteOffset : lay->byteOffset;
        LLVMValueRef offsetVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), byteOffset, 0);
        LLVMValueRef ptrI8 = LLVMBuildGEP2(ctx->builder, i8Ty, baseI8, &offsetVal, 1, "field.byte.gep");
        ptr = LLVMBuildBitCast(ctx->builder,
                               ptrI8,
                               LLVMPointerType(pointeeTy, 0),
                               "field.byte.ptr");
    }
    if (!ptr) {
        if (havePointedScratch) parsedTypeFree(&pointedScratch);
        return NULL;
    }
    if (isFlexArrayField && fieldLLVMType && LLVMGetTypeKind(fieldLLVMType) == LLVMArrayTypeKind) {
        LLVMTypeRef elemTy = LLVMGetElementType(fieldLLVMType);
        if (!elemTy || LLVMGetTypeKind(elemTy) == LLVMVoidTypeKind) {
            elemTy = LLVMInt8TypeInContext(ctx->llvmContext);
        }
        LLVMTypeRef i8Ty = LLVMInt8TypeInContext(ctx->llvmContext);
        LLVMValueRef baseI8 = LLVMBuildBitCast(ctx->builder, basePtr, LLVMPointerType(i8Ty, 0), "flex.base");

        uint64_t byteOffset = useByteOffsetOnly ? resolvedByteOffset : 0;
        if (!useByteOffsetOnly) {
            const CCTagFieldLayout* flexLay = cg_lookup_field_layout(ctx, structHint, fieldName);
            if (flexLay) {
                byteOffset = flexLay->byteOffset;
            }
        }
        LLVMValueRef offsetVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), byteOffset, 0);
        LLVMValueRef ptrI8 = LLVMBuildGEP2(ctx->builder, i8Ty, baseI8, &offsetVal, 1, "flex.byteptr");
        LLVMValueRef elemPtr = LLVMBuildBitCast(ctx->builder, ptrI8, LLVMPointerType(elemTy, 0), "flex.ptr");

        if (outFieldType) {
            *outFieldType = elemTy;
        }
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
