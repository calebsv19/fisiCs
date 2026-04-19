// SPDX-License-Identifier: Apache-2.0

#include "layout.h"
#include "type_checker.h"
#include "const_eval.h"
#include "target_layout.h"
#include "Compiler/compiler_context.h"
#include "Utils/profiler.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

static size_t round_up(size_t value, size_t align) {
    if (align == 0) return value;
    size_t rem = value % align;
    return rem == 0 ? value : value + (align - rem);
}

typedef struct {
    CCTagFieldLayout* items;
    size_t count;
    size_t capacity;
} FieldLayoutVec;

static void field_layout_vec_free(FieldLayoutVec* v) {
    if (!v || !v->items) return;
    for (size_t i = 0; i < v->count; ++i) {
        free(v->items[i].name);
    }
    free(v->items);
    v->items = NULL;
    v->count = v->capacity = 0;
}

static bool field_layout_vec_push(FieldLayoutVec* v, const CCTagFieldLayout* layout) {
    if (!v || !layout) return false;
    if (v->count == v->capacity) {
        size_t newCap = v->capacity < 8 ? 8 : v->capacity * 2;
        CCTagFieldLayout* n = (CCTagFieldLayout*)realloc(v->items, newCap * sizeof(CCTagFieldLayout));
        if (!n) return false;
        v->items = n;
        v->capacity = newCap;
    }
    v->items[v->count] = *layout;
    if (layout->name) {
        v->items[v->count].name = strdup(layout->name);
        if (!v->items[v->count].name) {
            return false;
        }
    }
    v->count++;
    return true;
}

static void collectAttrLayout(ASTAttribute* const* attrs,
                              size_t count,
                              bool* packed,
                              size_t* alignOverride);

static bool size_align_of_typeinfo(TypeInfo info, Scope* scope, size_t* sizeOut, size_t* alignOut);

static bool isFlexibleArrayType(const ParsedType* type) {
    if (!type || type->derivationCount == 0) return false;
    const TypeDerivation* last = parsedTypeGetDerivation(type, type->derivationCount - 1);
    return last &&
           last->kind == TYPE_DERIVATION_ARRAY &&
           last->as.array.isFlexible;
}

static bool size_align_of_array(TypeDerivation* arrayDeriv,
                                ParsedType* fullType,
                                Scope* scope,
                                size_t* sizeOut,
                                size_t* alignOut) {
    if (!arrayDeriv || !fullType) return false;
    if (arrayDeriv->as.array.isFlexible) return false;
    if (arrayDeriv->as.array.isVLA) return false;
    long long len = -1;
    if (arrayDeriv->as.array.hasConstantSize) {
        len = arrayDeriv->as.array.constantSize;
    } else if (arrayDeriv->as.array.sizeExpr) {
        if (!constEvalInteger(arrayDeriv->as.array.sizeExpr, scope, &len, true)) {
            return false;
        }
    }
    if (len < 0) return false;

    ParsedType element = parsedTypeArrayElementType(fullType);
    size_t elemSize = 0, elemAlign = 0;
    bool ok = size_align_of_parsed_type(&element, scope, &elemSize, &elemAlign);
    parsedTypeFree(&element);
    if (!ok) return false;
    if (sizeOut) *sizeOut = elemSize * (size_t)len;
    if (alignOut) *alignOut = elemAlign;
    return true;
}

bool size_align_of_parsed_type(ParsedType* type,
                               Scope* scope,
                               size_t* sizeOut,
                               size_t* alignOut) {
    if (!type) return false;
    bool packed = false;
    size_t alignOverride = 0;
    collectAttrLayout((ASTAttribute* const*)type->attributes, type->attributeCount, &packed, &alignOverride);
    if (type->hasAlignOverride && type->alignOverride > alignOverride) {
        alignOverride = type->alignOverride;
    }
    if (parsedTypeIsDirectArray(type)) {
        TypeDerivation* arr = parsedTypeGetMutableArrayDerivation(type, 0);
        size_t sz = 0, al = 0;
        if (!size_align_of_array(arr, type, scope, &sz, &al)) return false;
        if (packed) al = 1;
        if (alignOverride > 0 && alignOverride > al) al = alignOverride;
        if (sizeOut) *sizeOut = sz;
        if (alignOut) *alignOut = al;
        return true;
    }

    profiler_record_value("semantic_count_type_info_site_layout", 1);
    TypeInfo info = typeInfoFromParsedType(type, scope);
    size_t sz = 0, al = 0;
    if (!size_align_of_typeinfo(info, scope, &sz, &al)) return false;
    if (packed) al = 1;
    if (alignOverride > 0 && alignOverride > al) al = alignOverride;
    if (sizeOut) *sizeOut = sz;
    if (alignOut) *alignOut = al;
    return true;
}

static void sizeAlignFromBits(const TargetLayout* tl,
                              size_t bits,
                              size_t suggestedAlign,
                              size_t* outSize,
                              size_t* outAlign) {
    size_t bytes = (bits + 7) / 8;
    if (bytes == 0) bytes = 1;
    size_t align = suggestedAlign;
    if (align == 0 && tl) {
        if (bits <= tl->charBits) align = tl->charAlign;
        else if (bits <= tl->shortBits) align = tl->shortAlign;
        else if (bits <= tl->intBits) align = tl->intAlign;
        else if (bits <= tl->longBits) align = tl->longAlign;
        else align = tl->longLongAlign;
    }
    if (outSize) *outSize = bytes;
    if (outAlign) *outAlign = align;
}

static void collectAttrLayout(ASTAttribute* const* attrs,
                              size_t count,
                              bool* packed,
                              size_t* alignOverride) {
    if (!attrs) return;
    for (size_t i = 0; i < count; ++i) {
        const ASTAttribute* attr = attrs[i];
        if (!attr || !attr->payload) continue;
        const char* text = attr->payload;
        if (strstr(text, "packed")) {
            if (packed) *packed = true;
        }
        const char* aligned = strstr(text, "aligned");
        if (aligned) {
            const char* p = strchr(aligned, '(');
            if (p) {
                ++p;
                while (*p && isspace((unsigned char)*p)) ++p;
                char* endp = NULL;
                long val = strtol(p, &endp, 10);
                if (endp != p && val > 0) {
                    if (alignOverride && (size_t)val > *alignOverride) {
                        *alignOverride = (size_t)val;
                    }
                }
            }
        }
    }
}

static bool size_align_of_typeinfo(TypeInfo info, Scope* scope, size_t* sizeOut, size_t* alignOut) {
    const struct TargetLayout* tl = scope && scope->ctx ? cc_get_target_layout(scope->ctx) : tl_default();
    if (!tl) tl = tl_default();
    switch (info.category) {
        case TYPEINFO_INTEGER:
        case TYPEINFO_ENUM: {
            size_t bits = info.bitWidth ? info.bitWidth : 32;
            sizeAlignFromBits(tl, bits, 0, sizeOut, alignOut);
            return true;
        }
        case TYPEINFO_FLOAT: {
            size_t bits = info.bitWidth ? info.bitWidth : 32;
            size_t align = 0;
            if (info.bitWidth >= tl->longDoubleBits) {
                align = tl->longDoubleAlign;
            }
            size_t baseSize = 0;
            size_t baseAlign = 0;
            sizeAlignFromBits(tl, bits, align, &baseSize, &baseAlign);
            if (info.isComplex) {
                baseSize *= 2;
            }
            if (sizeOut) *sizeOut = baseSize;
            if (alignOut) *alignOut = baseAlign;
            return true;
        }
        case TYPEINFO_POINTER: {
            size_t bytes = tl->pointerBits / 8;
            size_t align = tl->pointerAlign;
            if (sizeOut) *sizeOut = bytes;
            if (alignOut) *alignOut = align;
            return true;
        }
        case TYPEINFO_STRUCT:
        case TYPEINFO_UNION: {
            if (!info.userTypeName || !scope || !scope->ctx) return false;
            CCTagKind kind = (info.category == TYPEINFO_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
            return layout_struct_union(scope->ctx, scope, kind, info.userTypeName, sizeOut, alignOut);
        }
        default:
            break;
    }
    return false;
}

static bool layout_struct_fields(ASTNode* def,
                                 Scope* scope,
                                 size_t* sizeOut,
                                 size_t* alignOut,
                                 FieldLayoutVec* layouts) {
    const char* dbgLayout = getenv("FISICS_DEBUG_LAYOUT");
    size_t offset = 0;
    size_t maxAlign = 1;
    size_t bitOffset = 0;       // bit position within current storage unit
    size_t currentUnitSize = 0; // bytes of the current bitfield storage unit
    if (!def) return false;
    const TargetLayout* tl = scope && scope->ctx ? cc_get_target_layout(scope->ctx) : tl_default();
    bool packed = false;
    size_t structAlignOverride = 0;
    collectAttrLayout(def->attributes, def->attributeCount, &packed, &structAlignOverride);
    if (dbgLayout) {
        fprintf(stderr, "[layout] struct attrs count=%zu packed=%d alignOverride=%zu\n",
                def->attributeCount, (int)packed, structAlignOverride);
    }
    ASTNode** fields = def->structDef.fields;
    for (size_t i = 0; i < def->structDef.fieldCount; ++i) {
        ASTNode* field = fields ? fields[i] : NULL;
        if (!field || field->type != AST_VARIABLE_DECLARATION) {
            continue;
        }
        bool isBitfield = field->varDecl.bitFieldWidth != NULL;
        bool fieldPacked = packed;
        size_t fieldAlignOverride = 0;
        collectAttrLayout(field->attributes, field->attributeCount, &fieldPacked, &fieldAlignOverride);
        ParsedType* baseType = &field->varDecl.declaredType;
        ParsedType* declaredTypes0 = field->varDecl.declaredTypes;
        if (declaredTypes0) {
            baseType = declaredTypes0;
        }
        const char* fieldName = NULL;
        ASTNode** varNames0 = field->varDecl.varNames;
        if (varNames0 && varNames0[0]) {
            fieldName = varNames0[0]->valueNode.value;
        }
        profiler_record_value("semantic_count_type_info_site_layout", 1);
        TypeInfo baseInfo = typeInfoFromParsedType(baseType, scope);

        if (isBitfield) {
            long long widthVal = 0;
            if (!constEvalInteger(field->varDecl.bitFieldWidth, scope, &widthVal, true)) {
                return false;
            }
            size_t fieldSize = 0, fieldAlign = 0;
            if (!size_align_of_parsed_type(baseType, scope, &fieldSize, &fieldAlign)) {
                return false;
            }
            if (fieldSize == 0) return false;
            if (dbgLayout) {
                fprintf(stderr,
                        "[layout] bitfield %s size=%zu align=%zu width=%lld\n",
                        fieldName ? fieldName : "<unnamed>",
                        fieldSize, fieldAlign, widthVal);
            }
            size_t unitBits = fieldSize * 8;
            size_t unitAlign = fieldAlign;
            if (tl && tl->bitfieldUnitBits > 0 && unitBits < tl->bitfieldUnitBits) {
                unitBits = tl->bitfieldUnitBits;
            }
            if (tl && tl->bitfieldUnitAlign > 0 && unitAlign < tl->bitfieldUnitAlign) {
                unitAlign = tl->bitfieldUnitAlign;
            }
            if (fieldPacked) unitAlign = 1;
            if (fieldAlignOverride > 0 && fieldAlignOverride > unitAlign) {
                unitAlign = fieldAlignOverride;
            }
            if (unitAlign == 0) unitAlign = 1;
            if (unitAlign > maxAlign) maxAlign = unitAlign;
            size_t unitBytes = (unitBits + 7) / 8;
            if (unitBytes == 0) unitBytes = 1;

            // Align to storage unit before placing bitfield
            offset = round_up(offset, unitAlign);
            if (widthVal == 0) {
                // Zero-width forces alignment to next storage unit boundary
                if (bitOffset > 0) {
                    offset += currentUnitSize;
                }
                bitOffset = 0;
                currentUnitSize = 0;
                offset = round_up(offset, unitAlign);
                if (layouts) {
                    CCTagFieldLayout lay = {
                        .name = fieldName ? strdup(fieldName) : NULL,
                        .byteOffset = offset,
                        .bitOffset = 0,
                        .widthBits = 0,
                        .storageUnitBytes = unitBytes,
                        .isBitfield = true,
                        .isZeroWidth = true,
                        .isSigned = baseInfo.isSigned
                    };
                    if (!field_layout_vec_push(layouts, &lay)) return false;
                    if (lay.name) free(lay.name);
                }
                continue;
            }
            size_t width = (size_t)widthVal;
            if (width > unitBits) return false;
            if (bitOffset == 0) {
                currentUnitSize = unitBytes;
            }
            // If not enough bits remain, move to next unit
            if (bitOffset + width > unitBits) {
                offset += currentUnitSize;
                bitOffset = 0;
                currentUnitSize = unitBytes;
            }
            size_t fieldByteOff = offset;
            size_t fieldBitOff = bitOffset;
            if (tl && tl->endianness == TL_ENDIAN_BIG && width > 0) {
                fieldBitOff = unitBits - width - bitOffset;
            }
            bitOffset += width;
            if (bitOffset == unitBits) {
                offset += currentUnitSize;
                bitOffset = 0;
                currentUnitSize = 0;
            }
            if (layouts) {
                CCTagFieldLayout lay = {
                    .name = fieldName ? strdup(fieldName) : NULL,
                    .byteOffset = fieldByteOff,
                    .bitOffset = fieldBitOff,
                    .widthBits = width,
                    .storageUnitBytes = unitBytes,
                    .isBitfield = true,
                    .isZeroWidth = false,
                    .isSigned = baseInfo.isSigned
                };
                if (!field_layout_vec_push(layouts, &lay)) return false;
                if (lay.name) free(lay.name);
            }
        } else {
            // Flush any in-progress bitfield storage before a non-bitfield
            if (bitOffset > 0) {
                offset += currentUnitSize;
                bitOffset = 0;
                currentUnitSize = 0;
            }

            bool flexible = isFlexibleArrayType(baseType);
            size_t fieldSize = 0, fieldAlign = 0;
            if (flexible) {
                ParsedType element = parsedTypeArrayElementType(baseType);
                bool ok = size_align_of_parsed_type(&element, scope, &fieldSize, &fieldAlign);
                parsedTypeFree(&element);
                if (!ok) return false;
            } else {
                bool ok = size_align_of_parsed_type(baseType, scope, &fieldSize, &fieldAlign);
                if (!ok) return false;
            }
            if (fieldAlign == 0) fieldAlign = 1;
            if (fieldPacked) fieldAlign = 1;
            if (fieldAlignOverride > 0 && fieldAlignOverride > fieldAlign) {
                fieldAlign = fieldAlignOverride;
            }
            offset = round_up(offset, fieldAlign);
            if (!flexible) {
                offset += fieldSize;
            }
            if (fieldAlign > maxAlign) {
                maxAlign = fieldAlign;
            }
            if (layouts) {
                CCTagFieldLayout lay = {
                    .name = fieldName ? strdup(fieldName) : NULL,
                    .byteOffset = offset - (flexible ? 0 : fieldSize),
                    .bitOffset = 0,
                    .widthBits = 0,
                    .storageUnitBytes = flexible ? 0 : fieldSize,
                    .isBitfield = false,
                    .isZeroWidth = false,
                    .isSigned = baseInfo.isSigned
                };
                if (!field_layout_vec_push(layouts, &lay)) return false;
                if (lay.name) free(lay.name);
            }
        }
        // handle multiple declarators
        if (field->varDecl.varCount > 1 && field->varDecl.declaredTypes) {
            ParsedType* declaredTypes = field->varDecl.declaredTypes;
            ASTNode** varNames = field->varDecl.varNames;
            for (size_t k = 1; k < field->varDecl.varCount; ++k) {
                if (bitOffset > 0) {
                    offset += currentUnitSize;
                    bitOffset = 0;
                    currentUnitSize = 0;
                }
                ParsedType* t = declaredTypes + k;
                ASTNode* nameNode = varNames ? varNames[k] : NULL;
                size_t sz = 0, al = 0;
                if (!size_align_of_parsed_type(t, scope, &sz, &al)) return false;
                if (al == 0) al = 1;
                if (fieldPacked) al = 1;
                if (fieldAlignOverride > 0 && fieldAlignOverride > al) al = fieldAlignOverride;
                offset = round_up(offset, al);
                offset += sz;
                if (al > maxAlign) maxAlign = al;
                if (layouts) {
                    const char* nm = nameNode ? nameNode->valueNode.value : NULL;
                    profiler_record_value("semantic_count_type_info_site_layout", 1);
                    TypeInfo info = typeInfoFromParsedType(t, scope);
                    CCTagFieldLayout lay = {
                        .name = nm ? strdup(nm) : NULL,
                        .byteOffset = offset - sz,
                        .bitOffset = 0,
                        .widthBits = 0,
                        .storageUnitBytes = sz,
                        .isBitfield = false,
                        .isZeroWidth = false,
                        .isSigned = info.isSigned
                    };
                    if (!field_layout_vec_push(layouts, &lay)) return false;
                    if (lay.name) free(lay.name);
                }
            }
        }
    }
    // Flush any remaining bitfield storage
    if (bitOffset > 0) {
        offset += currentUnitSize;
        bitOffset = 0;
        currentUnitSize = 0;
    }
    size_t finalAlign = maxAlign;
    if (packed && structAlignOverride == 0) {
        finalAlign = 1;
    }
    if (structAlignOverride > 0 && structAlignOverride > finalAlign) {
        finalAlign = structAlignOverride;
    }
    size_t finalSize = round_up(offset, finalAlign);
    if (sizeOut) *sizeOut = finalSize;
    if (alignOut) *alignOut = finalAlign;
    return true;
}

static bool layout_union_fields(ASTNode* def,
                                Scope* scope,
                                size_t* sizeOut,
                                size_t* alignOut,
                                FieldLayoutVec* layouts) {
    size_t maxSize = 0;
    size_t maxAlign = 1;
    if (!def) return false;
    bool packed = false;
    size_t structAlignOverride = 0;
    collectAttrLayout(def->attributes, def->attributeCount, &packed, &structAlignOverride);
    ASTNode** fields = def->structDef.fields;
    for (size_t i = 0; i < def->structDef.fieldCount; ++i) {
        ASTNode* field = fields ? fields[i] : NULL;
        if (!field || field->type != AST_VARIABLE_DECLARATION) continue;
        bool fieldPacked = packed;
        size_t fieldAlignOverride = 0;
        collectAttrLayout(field->attributes, field->attributeCount, &fieldPacked, &fieldAlignOverride);
        ParsedType* baseType = &field->varDecl.declaredType;
        ParsedType* declaredTypes0 = field->varDecl.declaredTypes;
        if (declaredTypes0) {
            baseType = declaredTypes0;
        }
        size_t fieldSize = 0, fieldAlign = 0;
        if (!size_align_of_parsed_type(baseType, scope, &fieldSize, &fieldAlign)) return false;
        if (fieldPacked) fieldAlign = 1;
        if (fieldAlignOverride > 0 && fieldAlignOverride > fieldAlign) {
            fieldAlign = fieldAlignOverride;
        }
        if (layouts) {
            const char* nm = NULL;
            ASTNode** varNames0 = field->varDecl.varNames;
            if (varNames0 && varNames0[0]) {
                nm = varNames0[0]->valueNode.value;
            }
            profiler_record_value("semantic_count_type_info_site_layout", 1);
            TypeInfo info = typeInfoFromParsedType(baseType, scope);
            CCTagFieldLayout lay = {
                .name = nm ? strdup(nm) : NULL,
                .byteOffset = 0,
                .bitOffset = 0,
                .widthBits = 0,
                .storageUnitBytes = fieldSize,
                .isBitfield = field->varDecl.bitFieldWidth != NULL,
                .isZeroWidth = false,
                .isSigned = info.isSigned
            };
            if (!field_layout_vec_push(layouts, &lay)) return false;
            if (lay.name) free(lay.name);
        }
        if (fieldSize > maxSize) maxSize = fieldSize;
        if (fieldAlign > maxAlign) maxAlign = fieldAlign;
        if (field->varDecl.varCount > 1 && field->varDecl.declaredTypes) {
            ParsedType* declaredTypes = field->varDecl.declaredTypes;
            ASTNode** varNames = field->varDecl.varNames;
            for (size_t k = 1; k < field->varDecl.varCount; ++k) {
                ParsedType* t = declaredTypes + k;
                size_t sz = 0, al = 0;
                if (!size_align_of_parsed_type(t, scope, &sz, &al)) return false;
                if (fieldPacked) al = 1;
                if (fieldAlignOverride > 0 && fieldAlignOverride > al) al = fieldAlignOverride;
                if (layouts) {
                    ASTNode* nameNode = varNames ? varNames[k] : NULL;
                    const char* nm2 = nameNode ? nameNode->valueNode.value : NULL;
                    profiler_record_value("semantic_count_type_info_site_layout", 1);
                    TypeInfo info2 = typeInfoFromParsedType(t, scope);
                    CCTagFieldLayout lay = {
                        .name = nm2 ? strdup(nm2) : NULL,
                        .byteOffset = 0,
                        .bitOffset = 0,
                        .widthBits = 0,
                        .storageUnitBytes = sz,
                        .isBitfield = field->varDecl.bitFieldWidth != NULL,
                        .isZeroWidth = false,
                        .isSigned = info2.isSigned
                    };
                    if (!field_layout_vec_push(layouts, &lay)) return false;
                    if (lay.name) free(lay.name);
                }
                if (sz > maxSize) maxSize = sz;
                if (al > maxAlign) maxAlign = al;
            }
        }
    }
    size_t finalAlign = maxAlign;
    if (packed && structAlignOverride == 0) {
        finalAlign = 1;
    }
    if (structAlignOverride > 0 && structAlignOverride > finalAlign) {
        finalAlign = structAlignOverride;
    }
    size_t finalSize = round_up(maxSize, finalAlign);
    if (sizeOut) *sizeOut = finalSize;
    if (alignOut) *alignOut = finalAlign;
    return true;
}

bool layout_struct_union(CompilerContext* ctx,
                         Scope* scope,
                         CCTagKind kind,
                         const char* name,
                         size_t* sizeOut,
                         size_t* alignOut) {
    if (!ctx || !name) return false;
    size_t cachedSize = 0, cachedAlign = 0;
    if (cc_get_tag_layout(ctx, kind, name, &cachedSize, &cachedAlign)) {
        if (sizeOut) *sizeOut = cachedSize;
        if (alignOut) *alignOut = cachedAlign;
        return true;
    }
    if (cc_tag_is_computing(ctx, kind, name)) {
        return false;
    }
    cc_tag_mark_computing(ctx, kind, name, true);
    ASTNode* def = cc_tag_definition(ctx, kind, name);
    if (!def) {
        cc_tag_mark_computing(ctx, kind, name, false);
        return false;
    }
    bool ok = false;
    size_t sz = 0, al = 0;
    FieldLayoutVec layouts = {0};
    if (kind == CC_TAG_STRUCT) {
        ok = layout_struct_fields(def, scope, &sz, &al, &layouts);
    } else {
        ok = layout_union_fields(def, scope, &sz, &al, &layouts);
    }
    if (ok) {
        cc_set_tag_layout(ctx, kind, name, sz, al);
        cc_set_tag_field_layouts(ctx, kind, name, layouts.items, layouts.count);
        const char* dbg = getenv("FISICS_DEBUG_LAYOUT");
        if (dbg) {
            fprintf(stderr, "[layout] %s %s size=%zu align=%zu\n",
                    (kind == CC_TAG_STRUCT ? "struct" : (kind == CC_TAG_UNION ? "union" : "enum")),
                    name, sz, al);
        }
        if (sizeOut) *sizeOut = sz;
        if (alignOut) *alignOut = al;
    }
    field_layout_vec_free(&layouts);
    cc_tag_mark_computing(ctx, kind, name, false);
    return ok;
}
