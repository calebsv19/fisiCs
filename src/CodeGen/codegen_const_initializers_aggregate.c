// SPDX-License-Identifier: Apache-2.0

#include "codegen_const_initializers_internal.h"

#include "codegen_types.h"

#include <stdlib.h>
#include <string.h>

static LLVMValueRef cg_merge_const_initializer(CodegenContext* ctx,
                                               LLVMValueRef baseConst,
                                               ASTNode* expr,
                                               LLVMTypeRef targetType,
                                               const ParsedType* parsedType);
static bool cg_find_field_in_definition(const ASTNode* def,
                                        const char* fieldName,
                                        unsigned* outIndex,
                                        const ParsedType** outParsed);
const CCTagFieldLayout* cg_init_lookup_field_layout(CodegenContext* ctx,
                                                    LLVMTypeRef aggregateType,
                                                    const ParsedType* aggregateParsed,
                                                    const char* fieldName);

static LLVMValueRef cg_zero_const(LLVMTypeRef type) {
    return type ? LLVMConstNull(type) : NULL;
}

static LLVMValueRef cg_const_bitfield_mask(LLVMTypeRef storageTy, unsigned width) {
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

static LLVMValueRef cg_merge_const_bitfield(CodegenContext* ctx,
                                            LLVMValueRef baseConst,
                                            const CCTagFieldLayout* lay,
                                            ASTNode* expr,
                                            LLVMTypeRef fieldType,
                                            const ParsedType* fieldParsed) {
    if (!ctx || !lay || !lay->isBitfield || lay->widthBits == 0 || !expr) {
        return NULL;
    }

    unsigned storageBits = (unsigned)(lay->storageUnitBytes ? lay->storageUnitBytes * 8 : 32);
    LLVMTypeRef storageTy = LLVMIntTypeInContext(ctx->llvmContext, storageBits);
    if (!storageTy) {
        return NULL;
    }

    LLVMValueRef oldConst = baseConst;
    if (!oldConst || LLVMTypeOf(oldConst) != storageTy) {
        oldConst = cg_zero_const(storageTy);
    }
    if (!oldConst || LLVMGetTypeKind(LLVMTypeOf(oldConst)) != LLVMIntegerTypeKind) {
        return NULL;
    }

    LLVMValueRef valueConst = cg_build_const_initializer(ctx, expr, fieldType, fieldParsed);
    if (!valueConst || LLVMGetTypeKind(LLVMTypeOf(valueConst)) != LLVMIntegerTypeKind) {
        return NULL;
    }

    uint64_t oldValue = LLVMConstIntGetZExtValue(oldConst);
    uint64_t newValue = LLVMConstIntGetZExtValue(valueConst);
    unsigned bitOffset = (unsigned)lay->bitOffset;
    uint64_t fieldMask = LLVMConstIntGetZExtValue(cg_const_bitfield_mask(storageTy, (unsigned)lay->widthBits));
    uint64_t shiftedMask = (bitOffset >= 64) ? 0ULL : (fieldMask << bitOffset);
    uint64_t shiftedValue = (bitOffset >= 64) ? 0ULL : ((newValue & fieldMask) << bitOffset);
    uint64_t combined = (oldValue & ~shiftedMask) | shiftedValue;
    return LLVMConstInt(storageTy, combined, 0);
}

static unsigned cg_const_bitfield_storage_index(CodegenContext* ctx,
                                                LLVMTypeRef structType,
                                                const CCTagFieldLayout* lay,
                                                unsigned fallbackIndex) {
    if (!ctx || !structType || !lay || LLVMGetTypeKind(structType) != LLVMStructTypeKind) {
        return fallbackIndex;
    }
    LLVMModuleRef module = cg_context_get_module(ctx);
    LLVMTargetDataRef td = module ? LLVMGetModuleDataLayout(module) : NULL;
    if (!td) {
        return fallbackIndex;
    }
    unsigned fieldCount = LLVMCountStructElementTypes(structType);
    for (unsigned i = 0; i < fieldCount; ++i) {
        if (LLVMOffsetOfElement(td, structType, i) == lay->byteOffset) {
            return i;
        }
    }
    return fallbackIndex;
}

static CGStructLLVMInfo* cg_find_struct_info(CodegenContext* ctx,
                                             LLVMTypeRef llvmType,
                                             const ParsedType* parsedType) {
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (!cache) return NULL;
    if (parsedType && parsedType->userTypeName) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(cache, parsedType->userTypeName);
        if (info) return info;
    }
    if (llvmType) {
        return cg_type_cache_find_struct_by_llvm(cache, llvmType);
    }
    return NULL;
}

static const StructInfo* cg_find_runtime_struct_info(CodegenContext* ctx,
                                                     LLVMTypeRef llvmType) {
    if (!ctx || !llvmType) return NULL;
    for (size_t i = 0; i < ctx->structInfoCount; ++i) {
        if (ctx->structInfos[i].llvmType == llvmType) {
            return &ctx->structInfos[i];
        }
    }
    return NULL;
}

static bool cg_entries_flat_scalars(DesignatedInit** entries, size_t entryCount) {
    if (!entries || entryCount == 0) return false;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;
        if (entry->indexExpr || entry->fieldName) {
            return false;
        }
        if (entry->expression->type == AST_COMPOUND_LITERAL) {
            return false;
        }
    }
    return true;
}

static bool cg_entries_have_designators(DesignatedInit** entries, size_t entryCount) {
    if (!entries || entryCount == 0) return false;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry) continue;
        if (entry->fieldName || entry->indexExpr) {
            return true;
        }
    }
    return false;
}

static LLVMValueRef cg_const_extract_aggregate_value(LLVMValueRef aggregateConst,
                                                     unsigned index,
                                                     LLVMTypeRef fallbackType) {
    if (!fallbackType) return NULL;
    if (!aggregateConst) {
        return cg_zero_const(fallbackType);
    }

    LLVMTypeRef aggregateType = LLVMTypeOf(aggregateConst);
    if (!aggregateType) {
        return cg_zero_const(fallbackType);
    }

    LLVMTypeKind kind = LLVMGetTypeKind(aggregateType);
    if (kind != LLVMArrayTypeKind && kind != LLVMStructTypeKind) {
        return cg_zero_const(fallbackType);
    }

    LLVMValueRef extracted = LLVMGetAggregateElement(aggregateConst, index);
    if (!extracted) {
        return cg_zero_const(fallbackType);
    }
    return extracted;
}

static bool cg_const_storage_byte_at(CodegenContext* ctx,
                                     LLVMValueRef storageConst,
                                     uint64_t index,
                                     unsigned char* outByte,
                                     LLVMTargetDataRef td);

static LLVMValueRef cg_const_pointer_at_storage_offset(LLVMValueRef sourceConst,
                                                       uint64_t offset,
                                                       uint64_t width,
                                                       LLVMTargetDataRef td) {
    if (!sourceConst || !td || width == 0) return NULL;
    LLVMTypeRef sourceType = LLVMTypeOf(sourceConst);
    if (!sourceType) return NULL;

    LLVMTypeKind kind = LLVMGetTypeKind(sourceType);
    if (kind == LLVMPointerTypeKind) {
        return offset == 0 && LLVMABISizeOfType(td, sourceType) == width
            ? sourceConst
            : NULL;
    }

    if (kind == LLVMArrayTypeKind) {
        LLVMTypeRef elemType = LLVMGetElementType(sourceType);
        uint64_t stride = LLVMABISizeOfType(td, elemType);
        unsigned length = LLVMGetArrayLength(sourceType);
        if (stride == 0 || offset / stride >= length ||
            offset % stride + width > stride) {
            return NULL;
        }
        LLVMValueRef elem = LLVMGetAggregateElement(sourceConst,
                                                    (unsigned)(offset / stride));
        return cg_const_pointer_at_storage_offset(elem,
                                                  offset % stride,
                                                  width,
                                                  td);
    }

    if (kind == LLVMStructTypeKind) {
        unsigned fieldCount = LLVMCountStructElementTypes(sourceType);
        for (unsigned i = 0; i < fieldCount; ++i) {
            LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(sourceType, i);
            uint64_t fieldOffset = LLVMOffsetOfElement(td, sourceType, i);
            uint64_t fieldSize = LLVMABISizeOfType(td, fieldType);
            if (fieldSize == 0 || offset < fieldOffset ||
                offset + width > fieldOffset + fieldSize) {
                continue;
            }
            LLVMValueRef field = LLVMGetAggregateElement(sourceConst, i);
            return cg_const_pointer_at_storage_offset(field,
                                                      offset - fieldOffset,
                                                      width,
                                                      td);
        }
    }

    return NULL;
}

static bool cg_const_pack_storage_bytes(CodegenContext* ctx,
                                        LLVMValueRef sourceConst,
                                        uint64_t baseOffset,
                                        unsigned byteCount,
                                        uint64_t* outPacked,
                                        LLVMTargetDataRef td) {
    if (!ctx || !sourceConst || !outPacked || !td || byteCount > 8) {
        return false;
    }
    LLVMTypeRef sourceType = LLVMTypeOf(sourceConst);
    uint64_t sourceSize = sourceType ? LLVMABISizeOfType(td, sourceType) : 0;
    uint64_t packed = 0;
    bool bigEndian = LLVMByteOrder(td) == LLVMBigEndian;
    for (unsigned i = 0; i < byteCount; ++i) {
        unsigned char value = 0;
        if (baseOffset + i < sourceSize &&
            !cg_const_storage_byte_at(ctx,
                                      sourceConst,
                                      baseOffset + i,
                                      &value,
                                      td)) {
            return false;
        }
        unsigned shiftIndex = bigEndian ? (byteCount - 1u - i) : i;
        packed |= ((uint64_t)value) << (8u * shiftIndex);
    }
    *outPacked = packed;
    return true;
}

static LLVMValueRef cg_const_storage_from_member_at(CodegenContext* ctx,
                                                    LLVMTypeRef storageType,
                                                    LLVMValueRef memberConst,
                                                    uint64_t memberOffset,
                                                    LLVMTargetDataRef td) {
    if (!ctx || !storageType || !memberConst || !td) return NULL;
    LLVMTypeKind kind = LLVMGetTypeKind(storageType);

    if (kind == LLVMIntegerTypeKind) {
        unsigned width = LLVMGetIntTypeWidth(storageType);
        if (width > 64 || width % 8 != 0) return NULL;
        unsigned byteCount = width / 8;
        LLVMValueRef sourcePointer =
            cg_const_pointer_at_storage_offset(memberConst,
                                               memberOffset,
                                               byteCount,
                                               td);
        if (sourcePointer) {
            return LLVMConstPtrToInt(sourcePointer, storageType);
        }
        uint64_t packed = 0;
        return cg_const_pack_storage_bytes(ctx,
                                           memberConst,
                                           memberOffset,
                                           byteCount,
                                           &packed,
                                           td)
            ? LLVMConstInt(storageType, packed, 0)
            : NULL;
    }

    if (kind == LLVMHalfTypeKind || kind == LLVMFloatTypeKind ||
        kind == LLVMDoubleTypeKind) {
        unsigned width = kind == LLVMHalfTypeKind
            ? 16u
            : (kind == LLVMFloatTypeKind ? 32u : 64u);
        uint64_t packed = 0;
        if (!cg_const_pack_storage_bytes(ctx,
                                         memberConst,
                                         memberOffset,
                                         width / 8,
                                         &packed,
                                         td)) {
            return NULL;
        }
        LLVMTypeRef bitsType = LLVMIntTypeInContext(LLVMGetTypeContext(storageType),
                                                    width);
        return LLVMConstBitCast(LLVMConstInt(bitsType, packed, 0), storageType);
    }

    if (kind == LLVMPointerTypeKind) {
        uint64_t byteCount = LLVMABISizeOfType(td, storageType);
        if (byteCount == 0 || byteCount > 8) return NULL;
        LLVMValueRef sourcePointer =
            cg_const_pointer_at_storage_offset(memberConst,
                                               memberOffset,
                                               byteCount,
                                               td);
        if (sourcePointer) {
            return LLVMTypeOf(sourcePointer) == storageType
                ? sourcePointer
                : LLVMConstPointerCast(sourcePointer, storageType);
        }
        uint64_t packed = 0;
        if (!cg_const_pack_storage_bytes(ctx,
                                         memberConst,
                                         memberOffset,
                                         (unsigned)byteCount,
                                         &packed,
                                         td)) {
            return NULL;
        }
        LLVMTypeRef bitsType = LLVMIntTypeInContext(LLVMGetTypeContext(storageType),
                                                    (unsigned)byteCount * 8u);
        return LLVMConstIntToPtr(LLVMConstInt(bitsType, packed, 0), storageType);
    }

    if (kind == LLVMArrayTypeKind) {
        LLVMTypeRef elemType = LLVMGetElementType(storageType);
        unsigned length = LLVMGetArrayLength(storageType);
        if (length == 0) return LLVMConstNull(storageType);
        uint64_t stride = LLVMABISizeOfType(td, elemType);
        if (stride == 0) return LLVMConstNull(storageType);
        LLVMValueRef* elems = (LLVMValueRef*)calloc(length, sizeof(LLVMValueRef));
        if (!elems) return NULL;
        bool ok = true;
        for (unsigned i = 0; i < length; ++i) {
            elems[i] = cg_const_storage_from_member_at(ctx,
                                                       elemType,
                                                       memberConst,
                                                       memberOffset +
                                                           (uint64_t)i * stride,
                                                       td);
            if (!elems[i]) {
                ok = false;
                break;
            }
        }
        LLVMValueRef result = ok ? LLVMConstArray(elemType, elems, length) : NULL;
        free(elems);
        return result;
    }

    if (kind == LLVMStructTypeKind) {
        unsigned fieldCount = LLVMCountStructElementTypes(storageType);
        LLVMValueRef* fields = (LLVMValueRef*)calloc(fieldCount,
                                                     sizeof(LLVMValueRef));
        if (!fields) return NULL;
        bool ok = true;
        for (unsigned i = 0; i < fieldCount; ++i) {
            LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(storageType, i);
            uint64_t fieldOffset = LLVMOffsetOfElement(td, storageType, i);
            fields[i] = cg_const_storage_from_member_at(ctx,
                                                        fieldType,
                                                        memberConst,
                                                        memberOffset + fieldOffset,
                                                        td);
            if (!fields[i]) {
                ok = false;
                break;
            }
        }
        LLVMValueRef result = ok
            ? LLVMConstNamedStruct(storageType, fields, fieldCount)
            : NULL;
        free(fields);
        return result;
    }

    return LLVMABISizeOfType(td, storageType) == 0
        ? LLVMConstNull(storageType)
        : NULL;
}

static LLVMValueRef cg_const_storage_from_exact_member(LLVMTypeRef storageType,
                                                       LLVMValueRef memberConst,
                                                       LLVMTargetDataRef td,
                                                       bool* outPlaced) {
    if (!storageType || !memberConst || !td || !outPlaced) return NULL;
    *outPlaced = false;
    LLVMTypeRef memberType = LLVMTypeOf(memberConst);
    if (storageType == memberType && LLVMABISizeOfType(td, storageType) > 0) {
        *outPlaced = true;
        return memberConst;
    }

    if (LLVMGetTypeKind(storageType) == LLVMArrayTypeKind &&
        LLVMGetArrayLength(storageType) == 0) {
        return LLVMConstNull(storageType);
    }
    if (LLVMGetTypeKind(storageType) != LLVMStructTypeKind) {
        return LLVMConstNull(storageType);
    }

    unsigned fieldCount = LLVMCountStructElementTypes(storageType);
    LLVMValueRef* fields = (LLVMValueRef*)calloc(fieldCount, sizeof(LLVMValueRef));
    if (!fields) return NULL;
    bool placed = false;
    for (unsigned i = 0; i < fieldCount; ++i) {
        LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(storageType, i);
        fields[i] = LLVMConstNull(fieldType);
        if (placed || LLVMOffsetOfElement(td, storageType, i) != 0 ||
            LLVMABISizeOfType(td, fieldType) == 0) {
            continue;
        }
        bool childPlaced = false;
        LLVMValueRef child = cg_const_storage_from_exact_member(fieldType,
                                                                memberConst,
                                                                td,
                                                                &childPlaced);
        if (child && childPlaced) {
            fields[i] = child;
            placed = true;
        }
    }
    LLVMValueRef result = LLVMConstNamedStruct(storageType, fields, fieldCount);
    free(fields);
    *outPlaced = placed;
    return result;
}

static LLVMValueRef cg_const_union_storage_from_member(CodegenContext* ctx,
                                                        LLVMTypeRef unionType,
                                                        LLVMValueRef memberConst) {
    if (!ctx || !unionType || !memberConst ||
        LLVMGetTypeKind(unionType) != LLVMStructTypeKind) {
        return NULL;
    }

    LLVMModuleRef module = cg_context_get_module(ctx);
    LLVMTargetDataRef td = module ? LLVMGetModuleDataLayout(module) : NULL;
    if (!td) return NULL;

    bool placedExactly = false;
    LLVMValueRef exactStorage = cg_const_storage_from_exact_member(unionType,
                                                                  memberConst,
                                                                  td,
                                                                  &placedExactly);
    if (exactStorage && placedExactly) {
        return exactStorage;
    }

    return cg_const_storage_from_member_at(ctx, unionType, memberConst, 0, td);
}

static bool cg_const_storage_byte_at(CodegenContext* ctx,
                                     LLVMValueRef storageConst,
                                     uint64_t index,
                                     unsigned char* outByte,
                                     LLVMTargetDataRef td) {
    if (!ctx || !storageConst || !outByte || !td) return false;
    LLVMTypeRef storageType = LLVMTypeOf(storageConst);
    if (!storageType) return false;

    LLVMTypeKind kind = LLVMGetTypeKind(storageType);
    if (kind == LLVMIntegerTypeKind) {
        unsigned width = LLVMGetIntTypeWidth(storageType);
        if (width > 64 || width % 8 != 0 || index >= width / 8) return false;
        uint64_t packed = LLVMConstIntGetZExtValue(storageConst);
        unsigned byteCount = width / 8;
        unsigned shiftIndex = LLVMByteOrder(td) == LLVMBigEndian
            ? (byteCount - 1u - (unsigned)index)
            : (unsigned)index;
        *outByte = (unsigned char)((packed >> (8u * shiftIndex)) & 0xffu);
        return true;
    }

    if (kind == LLVMHalfTypeKind || kind == LLVMFloatTypeKind ||
        kind == LLVMDoubleTypeKind) {
        unsigned width = kind == LLVMHalfTypeKind
            ? 16u
            : (kind == LLVMFloatTypeKind ? 32u : 64u);
        LLVMTypeRef bitsType = LLVMIntTypeInContext(LLVMGetTypeContext(storageType), width);
        LLVMValueRef bits = LLVMConstBitCast(storageConst, bitsType);
        return cg_const_storage_byte_at(ctx, bits, index, outByte, td);
    }

    if (kind == LLVMArrayTypeKind) {
        LLVMTypeRef elemType = LLVMGetElementType(storageType);
        unsigned length = LLVMGetArrayLength(storageType);
        uint64_t stride = LLVMABISizeOfType(td, elemType);
        if (length == 0 || stride == 0 || index >= (uint64_t)length * stride) {
            *outByte = 0;
            return true;
        }
        unsigned elemIndex = (unsigned)(index / stride);
        LLVMValueRef elem = LLVMGetAggregateElement(storageConst, elemIndex);
        if (!elem) return false;
        return cg_const_storage_byte_at(ctx, elem, index % stride, outByte, td);
    }

    if (kind == LLVMStructTypeKind) {
        unsigned fieldCount = LLVMCountStructElementTypes(storageType);
        for (unsigned i = 0; i < fieldCount; ++i) {
            LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(storageType, i);
            uint64_t offset = LLVMOffsetOfElement(td, storageType, i);
            uint64_t size = LLVMABISizeOfType(td, fieldType);
            if (size == 0 || index < offset || index >= offset + size) continue;
            LLVMValueRef field = LLVMGetAggregateElement(storageConst, i);
            return field && cg_const_storage_byte_at(ctx,
                                                     field,
                                                     index - offset,
                                                     outByte,
                                                     td);
        }
        *outByte = 0;
        return true;
    }

    return false;
}

static LLVMValueRef cg_const_i8_array_from_union_storage(CodegenContext* ctx,
                                                         LLVMValueRef storageConst,
                                                         LLVMTypeRef arrayType) {
    if (!ctx || !storageConst || !arrayType ||
        LLVMGetTypeKind(arrayType) != LLVMArrayTypeKind) {
        return NULL;
    }
    LLVMTypeRef elemType = LLVMGetElementType(arrayType);
    if (!elemType || LLVMGetTypeKind(elemType) != LLVMIntegerTypeKind ||
        LLVMGetIntTypeWidth(elemType) != 8) {
        return NULL;
    }

    LLVMModuleRef module = cg_context_get_module(ctx);
    LLVMTargetDataRef td = module ? LLVMGetModuleDataLayout(module) : NULL;
    if (!td) return NULL;
    unsigned length = LLVMGetArrayLength(arrayType);
    LLVMValueRef* elems = (LLVMValueRef*)calloc(length, sizeof(LLVMValueRef));
    if (!elems) return NULL;

    bool ok = true;
    for (unsigned i = 0; i < length; ++i) {
        unsigned char byte = 0;
        if (!cg_const_storage_byte_at(ctx, storageConst, i, &byte, td)) {
            ok = false;
            break;
        }
        elems[i] = LLVMConstInt(elemType, byte, 0);
    }

    LLVMValueRef result = ok ? LLVMConstArray(elemType, elems, length) : NULL;
    free(elems);
    return result;
}

static LLVMValueRef cg_merge_const_array(CodegenContext* ctx,
                                         LLVMValueRef baseConst,
                                         LLVMTypeRef arrayType,
                                         const ParsedType* parsedType,
                                         DesignatedInit** entries,
                                         size_t entryCount) {
    if (!ctx || !arrayType || LLVMGetTypeKind(arrayType) != LLVMArrayTypeKind) return NULL;

    unsigned length = LLVMGetArrayLength(arrayType);
    LLVMTypeRef elemType = LLVMGetElementType(arrayType);
    if (!elemType) return NULL;

    ParsedType elementParsed = {0};
    bool hasElementParsed = false;
    if (parsedType && parsedTypeIsDirectArray(parsedType)) {
        elementParsed = parsedTypeArrayElementType(parsedType);
        hasElementParsed = true;
        if (LLVMGetTypeKind(elemType) == LLVMStructTypeKind &&
            LLVMCountStructElementTypes(elemType) == 0) {
            LLVMTypeRef resolvedElemType = cg_type_from_parsed(ctx, &elementParsed);
            if (resolvedElemType &&
                LLVMGetTypeKind(resolvedElemType) == LLVMStructTypeKind &&
                LLVMCountStructElementTypes(resolvedElemType) == 0 &&
                elementParsed.userTypeName &&
                ctx->semanticModel) {
                CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                if (cctx) {
                    CCTagKind kind =
                        (elementParsed.kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                    ASTNode* def = cc_tag_definition(cctx, kind, elementParsed.userTypeName);
                    if (def) {
                        (void)codegenStructDefinition(ctx, def);
                        resolvedElemType = cg_type_from_parsed(ctx, &elementParsed);
                    }
                }
            }
            if (resolvedElemType &&
                LLVMGetTypeKind(resolvedElemType) == LLVMStructTypeKind &&
                LLVMCountStructElementTypes(resolvedElemType) > 0) {
                elemType = resolvedElemType;
            }
        }
    }

    LLVMValueRef* values = (LLVMValueRef*)calloc(length, sizeof(LLVMValueRef));
    if (!values) return NULL;
    for (unsigned i = 0; i < length; ++i) {
        values[i] = cg_const_extract_aggregate_value(baseConst, i, elemType);
    }

    unsigned long long implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned long long targetIndex = implicitIndex;
        if (entry->indexExpr) {
            bool ok = false;
            targetIndex = cg_eval_initializer_index_const(ctx, entry->indexExpr, &ok);
            if (!ok) {
                free(values);
                if (hasElementParsed) parsedTypeFree(&elementParsed);
                return NULL;
            }
        }
        implicitIndex = targetIndex + 1;
        if (targetIndex >= length) {
            continue;
        }

        LLVMValueRef baseElementConst = values[targetIndex];
        if (entry->resetSubobjectBeforeStore &&
            entry->expression->type == AST_COMPOUND_LITERAL) {
            baseElementConst = cg_zero_const(elemType);
        }
        LLVMValueRef elementConst = cg_merge_const_initializer(ctx,
                                                               baseElementConst,
                                                               entry->expression,
                                                               elemType,
                                                               hasElementParsed ? &elementParsed : NULL);
        if (!elementConst) {
            free(values);
            if (hasElementParsed) parsedTypeFree(&elementParsed);
            return NULL;
        }
        values[targetIndex] = elementConst;
    }

    LLVMValueRef result = LLVMConstArray(elemType, values, length);
    free(values);
    if (hasElementParsed) parsedTypeFree(&elementParsed);
    return result;
}

static LLVMValueRef cg_merge_const_struct(CodegenContext* ctx,
                                          LLVMValueRef baseConst,
                                          LLVMTypeRef structType,
                                          const ParsedType* parsedType,
                                          DesignatedInit** entries,
                                          size_t entryCount) {
    if (!ctx || !structType || LLVMGetTypeKind(structType) != LLVMStructTypeKind) return NULL;

    const ParsedType* resolvedType = parsedType ? cg_resolve_typedef_parsed(ctx, parsedType) : parsedType;
    const ParsedType* lookupType = resolvedType ? resolvedType : parsedType;
    CGStructLLVMInfo* info = cg_find_struct_info(ctx, structType, lookupType);
    const StructInfo* runtimeInfo = cg_find_runtime_struct_info(ctx, structType);
    bool isUnion = (lookupType && lookupType->kind == TYPE_UNION) ||
                   (info && info->isUnion) ||
                   (runtimeInfo && runtimeInfo->isUnion);

    unsigned fieldCount = LLVMCountStructElementTypes(structType);
    if (fieldCount == 0 && parsedType) {
        LLVMTypeRef resolved = cg_type_from_parsed(ctx, parsedType);
        if (resolved) {
            structType = resolved;
            fieldCount = LLVMCountStructElementTypes(structType);
        }
    }
    if (fieldCount == 0 && info && info->definition) {
        (void)codegenStructDefinition(ctx, (ASTNode*)info->definition);
        fieldCount = LLVMCountStructElementTypes(structType);
    }
    if (fieldCount == 0 && info && info->fieldCount > 0) {
        LLVMTypeRef* fieldTypes = (LLVMTypeRef*)calloc(info->fieldCount, sizeof(LLVMTypeRef));
        if (fieldTypes) {
            for (size_t i = 0; i < info->fieldCount; ++i) {
                fieldTypes[i] = cg_type_from_parsed(ctx, &info->fields[i].parsedType);
            }
            LLVMStructSetBody(structType, fieldTypes, (unsigned)info->fieldCount, 0);
            free(fieldTypes);
            fieldCount = LLVMCountStructElementTypes(structType);
        }
    }
    if (!info && lookupType && lookupType->userTypeName && ctx->semanticModel) {
        CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
        if (cctx) {
            CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
            ASTNode* def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
            if (def) {
                (void)codegenStructDefinition(ctx, def);
                info = cg_find_struct_info(ctx, structType, lookupType);
                LLVMTypeRef resolved = cg_type_from_parsed(ctx, lookupType);
                if (resolved) {
                    structType = resolved;
                    fieldCount = LLVMCountStructElementTypes(structType);
                }
            }
        }
    }
    if (fieldCount == 0 && parsedType && parsedType->userTypeName && ctx->semanticModel) {
        CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
        if (cctx) {
            CCTagKind kind = (parsedType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
            ASTNode* def = cc_tag_definition(cctx, kind, parsedType->userTypeName);
            if (def) {
                (void)codegenStructDefinition(ctx, def);
                structType = cg_type_from_parsed(ctx, parsedType);
                fieldCount = LLVMCountStructElementTypes(structType);
            }
        }
    }
    if (fieldCount == 0) {
        return NULL;
    }

    LLVMValueRef* fields = (LLVMValueRef*)calloc(fieldCount, sizeof(LLVMValueRef));
    if (!fields) return NULL;
    for (unsigned i = 0; i < fieldCount; ++i) {
        LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(structType, i);
        fields[i] = cg_const_extract_aggregate_value(baseConst, i, fieldType);
    }

    if (isUnion) {
        LLVMTypeRef firstType = LLVMStructGetTypeAtIndex(structType, 0);
        for (size_t i = 0; i < entryCount; ++i) {
            DesignatedInit* entry = entries[i];
            if (!entry || !entry->expression) continue;

            const ParsedType* fieldParsed = NULL;
            LLVMTypeRef fieldType = firstType;
            if (info && info->fieldCount > 0) {
                fieldParsed = &info->fields[0].parsedType;
            }
            if (entry->fieldName) {
                bool matchedField = false;
                if (info && info->fieldCount > 0) {
                    for (size_t f = 0; f < info->fieldCount; ++f) {
                        const char* fname = info->fields[f].name;
                        if (fname && strcmp(fname, entry->fieldName) == 0) {
                            fieldParsed = &info->fields[f].parsedType;
                            LLVMTypeRef parsedFieldType = cg_type_from_parsed(ctx, fieldParsed);
                            if (parsedFieldType) {
                                fieldType = parsedFieldType;
                            }
                            matchedField = true;
                            break;
                        }
                    }
                }
                if (!matchedField) {
                    const ASTNode* def = info ? (const ASTNode*)info->definition : NULL;
                    if (!def && lookupType && lookupType->userTypeName && ctx->semanticModel) {
                        CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                        if (cctx) {
                            CCTagKind kind =
                                (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                            def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
                        }
                    }
                    if (def) {
                        unsigned ignoredIndex = 0;
                        matchedField = cg_find_field_in_definition(def,
                                                                    entry->fieldName,
                                                                    &ignoredIndex,
                                                                    &fieldParsed);
                        if (matchedField) {
                            LLVMTypeRef parsedFieldType = cg_type_from_parsed(ctx, fieldParsed);
                            if (parsedFieldType) {
                                fieldType = parsedFieldType;
                            }
                        }
                    }
                }
            }

            LLVMValueRef storageConst = LLVMConstNamedStruct(structType, fields, fieldCount);
            LLVMValueRef baseMemberConst =
                cg_const_i8_array_from_union_storage(ctx, storageConst, fieldType);
            LLVMValueRef val = cg_merge_const_initializer(ctx,
                                                          baseMemberConst,
                                                          entry->expression,
                                                          fieldType,
                                                          fieldParsed);
            if (!val) {
                free(fields);
                return NULL;
            }
            if (LLVMTypeOf(val) != firstType) {
                LLVMValueRef storageVal =
                    cg_const_union_storage_from_member(ctx, structType, val);
                if (storageVal) {
                    fields[0] = LLVMGetAggregateElement(storageVal, 0);
                    for (unsigned f = 1; f < fieldCount; ++f) {
                        LLVMTypeRef storageFieldType = LLVMStructGetTypeAtIndex(structType, f);
                        LLVMValueRef storageField = LLVMGetAggregateElement(storageVal, f);
                        fields[f] = storageField ? storageField : cg_zero_const(storageFieldType);
                    }
                    continue;
                }
                free(fields);
                return NULL;
            }
            fields[0] = val;
            for (unsigned f = 1; f < fieldCount; ++f) {
                fields[f] = cg_zero_const(LLVMStructGetTypeAtIndex(structType, f));
            }
        }

        LLVMValueRef result = LLVMConstNamedStruct(structType, fields, fieldCount);
        free(fields);
        return result;
    }

    unsigned implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned targetLogicalIndex = implicitIndex;
        unsigned targetLLVMIndex = targetLogicalIndex;
        const ParsedType* fieldParsed = NULL;
        const char* targetFieldName = entry->fieldName;
        bool matchedField = false;
        if (entry->fieldName && info && info->fieldCount > 0) {
            for (size_t f = 0; f < info->fieldCount; ++f) {
                if (info->fields[f].name && strcmp(info->fields[f].name, entry->fieldName) == 0) {
                    targetLogicalIndex = info->fields[f].index;
                    targetLLVMIndex = info->fields[f].llvmIndex;
                    fieldParsed = &info->fields[f].parsedType;
                    targetFieldName = info->fields[f].name;
                    matchedField = true;
                    break;
                }
            }
        }
        if (entry->fieldName && !matchedField && runtimeInfo) {
            for (size_t f = 0; f < runtimeInfo->fieldCount; ++f) {
                if (runtimeInfo->fields[f].name &&
                    strcmp(runtimeInfo->fields[f].name, entry->fieldName) == 0) {
                    targetLogicalIndex = runtimeInfo->fields[f].index;
                    targetLLVMIndex = runtimeInfo->fields[f].llvmIndex;
                    fieldParsed = &runtimeInfo->fields[f].parsedType;
                    targetFieldName = runtimeInfo->fields[f].name;
                    matchedField = true;
                    break;
                }
            }
        }
        if (entry->fieldName && !matchedField) {
            const ASTNode* def = info ? (const ASTNode*)info->definition : NULL;
            if (!def && lookupType && lookupType->userTypeName && ctx->semanticModel) {
                CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                if (cctx) {
                    CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                    def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
                }
            }
            if (def) {
                matchedField = cg_find_field_in_definition(def,
                                                           entry->fieldName,
                                                           &targetLogicalIndex,
                                                           &fieldParsed);
            }
        }
        if (info && targetLogicalIndex < info->fieldCount) {
            targetLLVMIndex = info->fields[targetLogicalIndex].llvmIndex;
        } else if (runtimeInfo && targetLogicalIndex < runtimeInfo->fieldCount) {
            targetLLVMIndex = runtimeInfo->fields[targetLogicalIndex].llvmIndex;
            if (!fieldParsed) {
                fieldParsed = &runtimeInfo->fields[targetLogicalIndex].parsedType;
            }
        }
        if (!targetFieldName && info && targetLogicalIndex < info->fieldCount) {
            targetFieldName = info->fields[targetLogicalIndex].name;
        } else if (!targetFieldName && runtimeInfo &&
                   targetLogicalIndex < runtimeInfo->fieldCount) {
            targetFieldName = runtimeInfo->fields[targetLogicalIndex].name;
        }
        implicitIndex = targetLogicalIndex + 1;
        if (targetLLVMIndex >= fieldCount) {
            continue;
        }

        LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(structType, targetLLVMIndex);
        LLVMValueRef fieldConst = NULL;
        size_t mergedLast = i;
        DesignatedInit** mergedEntries = NULL;
        size_t mergedCount = 0;
        const CCTagFieldLayout* lay =
            cg_init_lookup_field_layout(ctx, structType, lookupType, targetFieldName);
        if (lay && lay->isBitfield && lay->widthBits > 0) {
            unsigned storageIndex =
                cg_const_bitfield_storage_index(ctx, structType, lay, targetLLVMIndex);
            LLVMTypeRef storageType = LLVMStructGetTypeAtIndex(structType, storageIndex);
            fieldConst = cg_merge_const_bitfield(ctx,
                                                 fields[storageIndex],
                                                 lay,
                                                 entry->expression,
                                                 storageType,
                                                 fieldParsed);
            if (!fieldConst) {
                free(fields);
                return NULL;
            }
            fields[storageIndex] = fieldConst;
            continue;
        }
        if (!entry->resetSubobjectBeforeStore &&
            targetFieldName &&
            entry->expression->type == AST_COMPOUND_LITERAL &&
            cg_entries_have_designators(entry->expression->compoundLiteral.entries,
                                        entry->expression->compoundLiteral.entryCount) &&
            (LLVMGetTypeKind(fieldType) == LLVMArrayTypeKind ||
             LLVMGetTypeKind(fieldType) == LLVMStructTypeKind)) {
            mergedCount = entry->expression->compoundLiteral.entryCount;
            for (size_t j = i + 1; j < entryCount; ++j) {
                DesignatedInit* next = entries[j];
                if (!next || !next->expression || !next->fieldName) {
                    break;
                }
                if (strcmp(next->fieldName, targetFieldName) != 0 ||
                    next->resetSubobjectBeforeStore ||
                    next->expression->type != AST_COMPOUND_LITERAL ||
                    !cg_entries_have_designators(next->expression->compoundLiteral.entries,
                                                next->expression->compoundLiteral.entryCount)) {
                    break;
                }
                mergedCount += next->expression->compoundLiteral.entryCount;
                mergedLast = j;
            }
            if (mergedLast > i) {
                mergedEntries = (DesignatedInit**)calloc(mergedCount, sizeof(DesignatedInit*));
                if (!mergedEntries) {
                    free(fields);
                    return NULL;
                }
                size_t cursor = 0;
                for (size_t j = i; j <= mergedLast; ++j) {
                    DesignatedInit* next = entries[j];
                    for (size_t k = 0; k < next->expression->compoundLiteral.entryCount; ++k) {
                        mergedEntries[cursor++] = next->expression->compoundLiteral.entries[k];
                    }
                }
            }
        }
        if (mergedEntries) {
            if (LLVMGetTypeKind(fieldType) == LLVMArrayTypeKind) {
                fieldConst = cg_build_const_array(ctx, fieldType, fieldParsed, mergedEntries, mergedCount);
            } else {
                fieldConst = cg_build_const_struct(ctx, fieldType, fieldParsed, mergedEntries, mergedCount);
            }
            free(mergedEntries);
            i = mergedLast;
        } else {
            LLVMValueRef baseFieldConst = fields[targetLLVMIndex];
            if (entry->resetSubobjectBeforeStore &&
                entry->expression->type == AST_COMPOUND_LITERAL) {
                baseFieldConst = cg_zero_const(fieldType);
            }
            fieldConst = cg_merge_const_initializer(ctx,
                                                    baseFieldConst,
                                                    entry->expression,
                                                    fieldType,
                                                    fieldParsed);
        }
        if (!fieldConst) {
            free(fields);
            return NULL;
        }
        fields[targetLLVMIndex] = fieldConst;
    }

    LLVMValueRef result = LLVMConstNamedStruct(structType, fields, fieldCount);
    free(fields);
    return result;
}

static LLVMValueRef cg_merge_const_initializer(CodegenContext* ctx,
                                               LLVMValueRef baseConst,
                                               ASTNode* expr,
                                               LLVMTypeRef targetType,
                                               const ParsedType* parsedType) {
    if (!ctx || !expr || !targetType) return NULL;
    if (expr->type == AST_COMPOUND_LITERAL &&
        cg_entries_have_designators(expr->compoundLiteral.entries,
                                    expr->compoundLiteral.entryCount)) {
        LLVMTypeKind targetKind = LLVMGetTypeKind(targetType);
        if (targetKind == LLVMArrayTypeKind) {
            return cg_merge_const_array(ctx,
                                        baseConst,
                                        targetType,
                                        parsedType,
                                        expr->compoundLiteral.entries,
                                        expr->compoundLiteral.entryCount);
        }
        if (targetKind == LLVMStructTypeKind) {
            return cg_merge_const_struct(ctx,
                                         baseConst,
                                         targetType,
                                         parsedType,
                                         expr->compoundLiteral.entries,
                                         expr->compoundLiteral.entryCount);
        }
    }
    return cg_build_const_initializer(ctx, expr, targetType, parsedType);
}

static LLVMValueRef cg_build_const_array_flat(CodegenContext* ctx,
                                              LLVMTypeRef arrayType,
                                              const ParsedType* parsedType,
                                              DesignatedInit** entries,
                                              size_t entryCount,
                                              size_t* cursor) {
    if (!ctx || !arrayType || LLVMGetTypeKind(arrayType) != LLVMArrayTypeKind || !cursor) {
        return NULL;
    }

    unsigned length = LLVMGetArrayLength(arrayType);
    LLVMTypeRef elemType = LLVMGetElementType(arrayType);
    if (!elemType) return NULL;

    LLVMValueRef* values = (LLVMValueRef*)calloc(length, sizeof(LLVMValueRef));
    if (!values) return NULL;

    ParsedType elementParsed = {0};
    bool hasElementParsed = false;
    if (parsedType && parsedTypeIsDirectArray(parsedType)) {
        elementParsed = parsedTypeArrayElementType(parsedType);
        hasElementParsed = true;
    }

    for (unsigned i = 0; i < length; ++i) {
        LLVMValueRef elemConst = cg_zero_const(elemType);
        if (*cursor < entryCount) {
            DesignatedInit* entry = entries[*cursor];
            if (entry && entry->expression) {
                if (LLVMGetTypeKind(elemType) == LLVMArrayTypeKind) {
                    if (entry->expression->type == AST_COMPOUND_LITERAL) {
                        elemConst = cg_build_const_initializer(ctx,
                                                               entry->expression,
                                                               elemType,
                                                               hasElementParsed ? &elementParsed : NULL);
                        (*cursor)++;
                    } else {
                        elemConst = cg_build_const_array_flat(ctx,
                                                              elemType,
                                                              hasElementParsed ? &elementParsed : NULL,
                                                              entries,
                                                              entryCount,
                                                              cursor);
                    }
                } else {
                    elemConst = cg_build_const_initializer(ctx,
                                                           entry->expression,
                                                           elemType,
                                                           hasElementParsed ? &elementParsed : NULL);
                    (*cursor)++;
                }
            }
        }
        if (!elemConst) {
            elemConst = cg_zero_const(elemType);
        }
        values[i] = elemConst;
    }

    LLVMValueRef result = LLVMConstArray(elemType, values, length);
    free(values);
    if (hasElementParsed) parsedTypeFree(&elementParsed);
    return result;
}

LLVMValueRef cg_build_const_array(CodegenContext* ctx,
                                  LLVMTypeRef arrayType,
                                  const ParsedType* parsedType,
                                  DesignatedInit** entries,
                                  size_t entryCount) {
    if (!ctx || !arrayType || LLVMGetTypeKind(arrayType) != LLVMArrayTypeKind) return NULL;

    if (entryCount == 0) {
        return cg_zero_const(arrayType);
    }

    if (cg_entries_flat_scalars(entries, entryCount)) {
        size_t cursor = 0;
        return cg_build_const_array_flat(ctx, arrayType, parsedType, entries, entryCount, &cursor);
    }

    unsigned length = LLVMGetArrayLength(arrayType);
    LLVMTypeRef elemType = LLVMGetElementType(arrayType);
    if (!elemType) return NULL;

    LLVMValueRef* values = (LLVMValueRef*)calloc(length, sizeof(LLVMValueRef));
    if (!values) return NULL;

    ParsedType elementParsed = {0};
    bool hasElementParsed = false;
    if (parsedType && parsedTypeIsDirectArray(parsedType)) {
        elementParsed = parsedTypeArrayElementType(parsedType);
        hasElementParsed = true;
    }

    for (unsigned i = 0; i < length; ++i) {
        values[i] = cg_zero_const(elemType);
    }

    unsigned long long implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned long long targetIndex = implicitIndex;
        if (entry->indexExpr) {
            bool ok = false;
            targetIndex = cg_eval_initializer_index_const(ctx, entry->indexExpr, &ok);
            if (!ok) {
                free(values);
                if (hasElementParsed) parsedTypeFree(&elementParsed);
                return NULL;
            }
        }
        implicitIndex = targetIndex + 1;
        if (targetIndex >= length) {
            continue;
        }

        LLVMValueRef baseElementConst = values[targetIndex];
        if (entry->resetSubobjectBeforeStore &&
            entry->expression->type == AST_COMPOUND_LITERAL) {
            baseElementConst = cg_zero_const(elemType);
        }
        LLVMValueRef elementConst = cg_merge_const_initializer(ctx,
                                                               baseElementConst,
                                                               entry->expression,
                                                               elemType,
                                                               hasElementParsed ? &elementParsed : NULL);
        if (!elementConst) {
            free(values);
            if (hasElementParsed) parsedTypeFree(&elementParsed);
            return NULL;
        }
        values[targetIndex] = elementConst;
    }

    LLVMValueRef result = LLVMConstArray(elemType, values, length);
    free(values);
    if (hasElementParsed) parsedTypeFree(&elementParsed);
    return result;
}

static bool cg_find_field_in_definition(const ASTNode* def,
                                        const char* fieldName,
                                        unsigned* outIndex,
                                        const ParsedType** outParsed) {
    if (!def || !fieldName || !outIndex) return false;
    if (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION) {
        return false;
    }
    unsigned index = 0;
    for (size_t f = 0; f < def->structDef.fieldCount; ++f) {
        ASTNode* fieldDecl = def->structDef.fields[f];
        if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
        for (size_t v = 0; v < fieldDecl->varDecl.varCount; ++v) {
            ASTNode* nameNode = fieldDecl->varDecl.varNames[v];
            const char* candidate = (nameNode && nameNode->type == AST_IDENTIFIER)
                ? nameNode->valueNode.value
                : NULL;
            if (candidate && strcmp(candidate, fieldName) == 0) {
                *outIndex = (def->type == AST_UNION_DEFINITION) ? 0u : index;
                if (outParsed) {
                    const ParsedType* parsed = astVarDeclTypeAt((ASTNode*)fieldDecl, v);
                    *outParsed = parsed ? parsed : &fieldDecl->varDecl.declaredType;
                }
                return true;
            }
            if (def->type != AST_UNION_DEFINITION) {
                ++index;
            }
        }
    }
    return false;
}

LLVMValueRef cg_build_const_struct(CodegenContext* ctx,
                                   LLVMTypeRef structType,
                                   const ParsedType* parsedType,
                                   DesignatedInit** entries,
                                   size_t entryCount) {
    if (!ctx || !structType || LLVMGetTypeKind(structType) != LLVMStructTypeKind) return NULL;

    const ParsedType* resolvedType = parsedType ? cg_resolve_typedef_parsed(ctx, parsedType) : parsedType;
    const ParsedType* lookupType = resolvedType ? resolvedType : parsedType;
    bool isUnion = lookupType && lookupType->kind == TYPE_UNION;
    CGStructLLVMInfo* info = cg_find_struct_info(ctx, structType, lookupType);
    const StructInfo* runtimeInfo = cg_find_runtime_struct_info(ctx, structType);
    if (info && info->isUnion) {
        isUnion = true;
    }
    if (runtimeInfo && runtimeInfo->isUnion) {
        isUnion = true;
    }

    unsigned fieldCount = LLVMCountStructElementTypes(structType);
    if (fieldCount == 0 && parsedType) {
        LLVMTypeRef resolved = cg_type_from_parsed(ctx, parsedType);
        if (resolved) {
            structType = resolved;
            fieldCount = LLVMCountStructElementTypes(structType);
        }
    }
    if (fieldCount == 0 && info && info->definition) {
        (void)codegenStructDefinition(ctx, (ASTNode*)info->definition);
        fieldCount = LLVMCountStructElementTypes(structType);
    }
    if (fieldCount == 0 && info && info->fieldCount > 0) {
        LLVMTypeRef* fieldTypes = (LLVMTypeRef*)calloc(info->fieldCount, sizeof(LLVMTypeRef));
        if (fieldTypes) {
            for (size_t i = 0; i < info->fieldCount; ++i) {
                fieldTypes[i] = cg_type_from_parsed(ctx, &info->fields[i].parsedType);
            }
            LLVMStructSetBody(structType, fieldTypes, (unsigned)info->fieldCount, 0);
            free(fieldTypes);
            fieldCount = LLVMCountStructElementTypes(structType);
        }
    }
    if (!info && lookupType && lookupType->userTypeName && ctx->semanticModel) {
        CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
        if (cctx) {
            CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
            ASTNode* def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
            if (def) {
                (void)codegenStructDefinition(ctx, def);
                info = cg_find_struct_info(ctx, structType, lookupType);
            }
        }
    }

    if (fieldCount == 0 && parsedType && parsedType->userTypeName && ctx->semanticModel) {
        CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
        if (cctx) {
            CCTagKind kind = (parsedType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
            ASTNode* def = cc_tag_definition(cctx, kind, parsedType->userTypeName);
            if (def) {
                (void)codegenStructDefinition(ctx, def);
                structType = cg_type_from_parsed(ctx, parsedType);
                fieldCount = LLVMCountStructElementTypes(structType);
            }
        }
    }
    if (fieldCount == 0) {
        return LLVMConstStruct(NULL, 0, 0);
    }

    LLVMValueRef* fields = (LLVMValueRef*)calloc(fieldCount, sizeof(LLVMValueRef));
    if (!fields) return NULL;
    for (unsigned i = 0; i < fieldCount; ++i) {
        fields[i] = cg_zero_const(LLVMStructGetTypeAtIndex(structType, i));
    }

    if (isUnion) {
        if (entryCount > 0 && entries[0] && entries[0]->expression) {
            LLVMTypeRef firstType = LLVMStructGetTypeAtIndex(structType, 0);
            const ParsedType* fieldParsed = NULL;
            LLVMTypeRef fieldType = firstType;
            if (info && info->fieldCount > 0) {
                fieldParsed = &info->fields[0].parsedType;
            }
            if (entries[0]->fieldName && info) {
                for (size_t f = 0; f < info->fieldCount; ++f) {
                    const char* fname = info->fields[f].name;
                    if (fname && strcmp(fname, entries[0]->fieldName) == 0) {
                        fieldParsed = &info->fields[f].parsedType;
                        fieldType = cg_type_from_parsed(ctx, fieldParsed);
                        if (!fieldType) {
                            fieldType = firstType;
                        }
                        break;
                    }
                }
            }
            if (entries[0]->fieldName && !info && lookupType && lookupType->userTypeName && ctx->semanticModel) {
                CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                if (cctx) {
                    CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                    ASTNode* def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
                    if (def && (def->type == AST_UNION_DEFINITION || def->type == AST_STRUCT_DEFINITION)) {
                        for (size_t f = 0; f < def->structDef.fieldCount; ++f) {
                            ASTNode* fieldDecl = def->structDef.fields[f];
                            if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
                            for (size_t v = 0; v < fieldDecl->varDecl.varCount; ++v) {
                                ASTNode* nameNode = fieldDecl->varDecl.varNames[v];
                                const char* fname = (nameNode && nameNode->type == AST_IDENTIFIER)
                                    ? nameNode->valueNode.value
                                    : NULL;
                                if (fname && strcmp(fname, entries[0]->fieldName) == 0) {
                                    const ParsedType* parsed = astVarDeclTypeAt(fieldDecl, v);
                                    fieldParsed = parsed ? parsed : &fieldDecl->varDecl.declaredType;
                                    fieldType = cg_type_from_parsed(ctx, fieldParsed);
                                    if (!fieldType) {
                                        fieldType = firstType;
                                    }
                                    f = def->structDef.fieldCount;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            LLVMValueRef val = cg_build_const_initializer(ctx, entries[0]->expression, fieldType, fieldParsed);
            if (val && LLVMTypeOf(val) != firstType) {
                LLVMValueRef storageVal =
                    cg_const_union_storage_from_member(ctx, structType, val);
                if (storageVal) {
                    free(fields);
                    return storageVal;
                }
                free(fields);
                return NULL;
            }
            if (val) {
                fields[0] = val;
            }
        }
        LLVMValueRef result = LLVMConstNamedStruct(structType, fields, fieldCount);
        free(fields);
        return result;
    }

    unsigned implicitIndex = 0;
    for (size_t i = 0; i < entryCount; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry || !entry->expression) continue;

        unsigned targetLogicalIndex = implicitIndex;
        unsigned targetLLVMIndex = targetLogicalIndex;
        const ParsedType* fieldParsed = NULL;
        const char* targetFieldName = entry->fieldName;

        bool matchedField = false;
        if (entry->fieldName && info && info->fieldCount > 0) {
            for (size_t f = 0; f < info->fieldCount; ++f) {
                if (info->fields[f].name && strcmp(info->fields[f].name, entry->fieldName) == 0) {
                    targetLogicalIndex = info->fields[f].index;
                    targetLLVMIndex = info->fields[f].llvmIndex;
                    fieldParsed = &info->fields[f].parsedType;
                    targetFieldName = info->fields[f].name;
                    matchedField = true;
                    break;
                }
            }
        }
        if (entry->fieldName && !matchedField && runtimeInfo) {
            for (size_t f = 0; f < runtimeInfo->fieldCount; ++f) {
                if (runtimeInfo->fields[f].name &&
                    strcmp(runtimeInfo->fields[f].name, entry->fieldName) == 0) {
                    targetLogicalIndex = runtimeInfo->fields[f].index;
                    targetLLVMIndex = runtimeInfo->fields[f].llvmIndex;
                    fieldParsed = &runtimeInfo->fields[f].parsedType;
                    targetFieldName = runtimeInfo->fields[f].name;
                    matchedField = true;
                    break;
                }
            }
        }
        if (entry->fieldName && !matchedField) {
            const ASTNode* def = info ? (const ASTNode*)info->definition : NULL;
            if (!def && lookupType && lookupType->userTypeName && ctx->semanticModel) {
                CompilerContext* cctx = semanticModelGetContext(ctx->semanticModel);
                if (cctx) {
                    CCTagKind kind = (lookupType->kind == TYPE_UNION) ? CC_TAG_UNION : CC_TAG_STRUCT;
                    def = cc_tag_definition(cctx, kind, lookupType->userTypeName);
                }
            }
            if (def) {
                matchedField = cg_find_field_in_definition(def,
                                                           entry->fieldName,
                                                           &targetLogicalIndex,
                                                           &fieldParsed);
            }
        }
        if (info && targetLogicalIndex < info->fieldCount) {
            targetLLVMIndex = info->fields[targetLogicalIndex].llvmIndex;
        } else if (runtimeInfo && targetLogicalIndex < runtimeInfo->fieldCount) {
            targetLLVMIndex = runtimeInfo->fields[targetLogicalIndex].llvmIndex;
            if (!fieldParsed) {
                fieldParsed = &runtimeInfo->fields[targetLogicalIndex].parsedType;
            }
        }
        if (!targetFieldName && info && targetLogicalIndex < info->fieldCount) {
            targetFieldName = info->fields[targetLogicalIndex].name;
        } else if (!targetFieldName && runtimeInfo &&
                   targetLogicalIndex < runtimeInfo->fieldCount) {
            targetFieldName = runtimeInfo->fields[targetLogicalIndex].name;
        }
        implicitIndex = targetLogicalIndex + 1;

        if (targetLLVMIndex >= fieldCount) {
            continue;
        }

        LLVMTypeRef fieldType = LLVMStructGetTypeAtIndex(structType, targetLLVMIndex);
        LLVMValueRef baseFieldConst = fields[targetLLVMIndex];
        const CCTagFieldLayout* lay =
            cg_init_lookup_field_layout(ctx, structType, lookupType, targetFieldName);
        if (lay && lay->isBitfield && lay->widthBits > 0) {
            unsigned storageIndex =
                cg_const_bitfield_storage_index(ctx, structType, lay, targetLLVMIndex);
            LLVMTypeRef storageType = LLVMStructGetTypeAtIndex(structType, storageIndex);
            LLVMValueRef fieldConst = cg_merge_const_bitfield(ctx,
                                                              fields[storageIndex],
                                                              lay,
                                                              entry->expression,
                                                              storageType,
                                                              fieldParsed);
            if (!fieldConst) {
                free(fields);
                return NULL;
            }
            fields[storageIndex] = fieldConst;
            continue;
        }
        if (entry->resetSubobjectBeforeStore &&
            entry->expression->type == AST_COMPOUND_LITERAL) {
            baseFieldConst = cg_zero_const(fieldType);
        }
        LLVMValueRef fieldConst = cg_merge_const_initializer(ctx,
                                                             baseFieldConst,
                                                             entry->expression,
                                                             fieldType,
                                                             fieldParsed);
        if (!fieldConst) {
            free(fields);
            return NULL;
        }
        fields[targetLLVMIndex] = fieldConst;
    }

    LLVMValueRef result = LLVMConstNamedStruct(structType, fields, fieldCount);
    free(fields);
    return result;
}
