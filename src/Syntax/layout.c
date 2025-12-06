#include "layout.h"
#include "type_checker.h"
#include "const_eval.h"
#include <string.h>
#include <ctype.h>

static size_t round_up(size_t value, size_t align) {
    if (align == 0) return value;
    size_t rem = value % align;
    return rem == 0 ? value : value + (align - rem);
}

static void collectAttrLayout(ASTAttribute* const* attrs,
                              size_t count,
                              bool* packed,
                              size_t* alignOverride);

static bool size_align_of_typeinfo(TypeInfo info, Scope* scope, size_t* sizeOut, size_t* alignOut);

static bool size_align_of_array(TypeDerivation* arrayDeriv,
                                ParsedType* fullType,
                                Scope* scope,
                                size_t* sizeOut,
                                size_t* alignOut) {
    if (!arrayDeriv || !fullType) return false;
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

    TypeInfo info = typeInfoFromParsedType(type, scope);
    size_t sz = 0, al = 0;
    if (!size_align_of_typeinfo(info, scope, &sz, &al)) return false;
    if (packed) al = 1;
    if (alignOverride > 0 && alignOverride > al) al = alignOverride;
    if (sizeOut) *sizeOut = sz;
    if (alignOut) *alignOut = al;
    return true;
}

typedef struct {
    size_t charBits, shortBits, intBits, longBits, longLongBits;
    size_t floatBits, doubleBits, pointerBits;
    size_t charAlign, shortAlign, intAlign, longAlign, longLongAlign;
    size_t floatAlign, doubleAlign, pointerAlign;
} AbiProfile;

static const AbiProfile abi_lp64 = {
    .charBits = 8,  .charAlign = 1,
    .shortBits = 16, .shortAlign = 2,
    .intBits = 32,  .intAlign = 4,
    .longBits = 64, .longAlign = 8,
    .longLongBits = 64, .longLongAlign = 8,
    .floatBits = 32, .floatAlign = 4,
    .doubleBits = 64, .doubleAlign = 8,
    .pointerBits = 64, .pointerAlign = 8,
};

static const AbiProfile abi_llp64 = {
    .charBits = 8,  .charAlign = 1,
    .shortBits = 16, .shortAlign = 2,
    .intBits = 32,  .intAlign = 4,
    .longBits = 32, .longAlign = 4,
    .longLongBits = 64, .longLongAlign = 8,
    .floatBits = 32, .floatAlign = 4,
    .doubleBits = 64, .doubleAlign = 8,
    .pointerBits = 64, .pointerAlign = 8,
};

static const AbiProfile* currentAbi(void) {
#ifdef TARGET_ABI_LLP64
    return &abi_llp64;
#else
    return &abi_lp64;
#endif
}

static void sizeAlignFromBits(const AbiProfile* abi,
                              size_t bits,
                              size_t suggestedAlign,
                              size_t* outSize,
                              size_t* outAlign) {
    size_t bytes = (bits + 7) / 8;
    if (bytes == 0) bytes = 1;
    size_t align = suggestedAlign;
    if (align == 0) {
        if (bits <= abi->charBits) align = abi->charAlign;
        else if (bits <= abi->shortBits) align = abi->shortAlign;
        else if (bits <= abi->intBits) align = abi->intAlign;
        else if (bits <= abi->longBits) align = abi->longAlign;
        else align = abi->longLongAlign;
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
    const AbiProfile* abi = currentAbi();
    switch (info.category) {
        case TYPEINFO_INTEGER:
        case TYPEINFO_ENUM: {
            size_t bits = info.bitWidth ? info.bitWidth : 32;
            sizeAlignFromBits(abi, bits, 0, sizeOut, alignOut);
            return true;
        }
        case TYPEINFO_FLOAT: {
            size_t bits = info.bitWidth ? info.bitWidth : 32;
            sizeAlignFromBits(abi, bits, 0, sizeOut, alignOut);
            return true;
        }
        case TYPEINFO_POINTER: {
            size_t bytes = abi->pointerBits / 8;
            size_t align = abi->pointerAlign;
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
                                 size_t* alignOut) {
    size_t offset = 0;
    size_t maxAlign = 1;
    size_t bitOffset = 0;       // bit position within current storage unit
    size_t currentUnitSize = 0; // bytes of the current bitfield storage unit
    if (!def) return false;
    bool packed = false;
    size_t structAlignOverride = 0;
    collectAttrLayout(def->attributes, def->attributeCount, &packed, &structAlignOverride);
    for (size_t i = 0; i < def->structDef.fieldCount; ++i) {
        ASTNode* field = def->structDef.fields ? def->structDef.fields[i] : NULL;
        if (!field || field->type != AST_VARIABLE_DECLARATION) {
            continue;
        }
        bool isBitfield = field->varDecl.bitFieldWidth != NULL;
        bool fieldPacked = packed;
        size_t fieldAlignOverride = 0;
        collectAttrLayout(field->attributes, field->attributeCount, &fieldPacked, &fieldAlignOverride);
        ParsedType* baseType = field->varDecl.declaredTypes
            ? &field->varDecl.declaredTypes[0]
            : &field->varDecl.declaredType;

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
            size_t unitBits = fieldSize * 8;
            if (fieldPacked) fieldAlign = 1;
            if (fieldAlignOverride > 0 && fieldAlignOverride > fieldAlign) {
                fieldAlign = fieldAlignOverride;
            }
            if (fieldAlign == 0) fieldAlign = 1;
            if (fieldAlign > maxAlign) maxAlign = fieldAlign;

            // Align to storage unit before placing bitfield
            offset = round_up(offset, fieldAlign);
            if (widthVal == 0) {
                // Zero-width forces alignment to next storage unit boundary
                if (bitOffset > 0) {
                    offset += currentUnitSize;
                }
                bitOffset = 0;
                currentUnitSize = 0;
                continue;
            }
            size_t width = (size_t)widthVal;
            if (width > unitBits) return false;
            if (bitOffset == 0) {
                currentUnitSize = fieldSize;
            }
            // If not enough bits remain, move to next unit
            if (bitOffset + width > unitBits) {
                offset += currentUnitSize;
                bitOffset = 0;
                currentUnitSize = fieldSize;
            }
            bitOffset += width;
            if (bitOffset == unitBits) {
                offset += currentUnitSize;
                bitOffset = 0;
                currentUnitSize = 0;
            }
        } else {
            // Flush any in-progress bitfield storage before a non-bitfield
            if (bitOffset > 0) {
                offset += currentUnitSize;
                bitOffset = 0;
                currentUnitSize = 0;
            }

            size_t fieldSize = 0, fieldAlign = 0;
            bool ok = size_align_of_parsed_type(baseType, scope, &fieldSize, &fieldAlign);
            if (!ok) return false;
            if (fieldAlign == 0) fieldAlign = 1;
            if (fieldPacked) fieldAlign = 1;
            if (fieldAlignOverride > 0 && fieldAlignOverride > fieldAlign) {
                fieldAlign = fieldAlignOverride;
            }
            offset = round_up(offset, fieldAlign);
            offset += fieldSize;
            if (fieldAlign > maxAlign) {
                maxAlign = fieldAlign;
            }
        }
        // handle multiple declarators
        if (field->varDecl.varCount > 1 && field->varDecl.declaredTypes) {
            for (size_t k = 1; k < field->varDecl.varCount; ++k) {
                if (bitOffset > 0) {
                    offset += currentUnitSize;
                    bitOffset = 0;
                    currentUnitSize = 0;
                }
                ParsedType* t = &field->varDecl.declaredTypes[k];
                size_t sz = 0, al = 0;
                if (!size_align_of_parsed_type(t, scope, &sz, &al)) return false;
                if (al == 0) al = 1;
                if (fieldPacked) al = 1;
                if (fieldAlignOverride > 0 && fieldAlignOverride > al) al = fieldAlignOverride;
                offset = round_up(offset, al);
                offset += sz;
                if (al > maxAlign) maxAlign = al;
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
                                size_t* alignOut) {
    size_t maxSize = 0;
    size_t maxAlign = 1;
    if (!def) return false;
    bool packed = false;
    size_t structAlignOverride = 0;
    collectAttrLayout(def->attributes, def->attributeCount, &packed, &structAlignOverride);
    for (size_t i = 0; i < def->structDef.fieldCount; ++i) {
        ASTNode* field = def->structDef.fields ? def->structDef.fields[i] : NULL;
        if (!field || field->type != AST_VARIABLE_DECLARATION) continue;
        bool fieldPacked = packed;
        size_t fieldAlignOverride = 0;
        collectAttrLayout(field->attributes, field->attributeCount, &fieldPacked, &fieldAlignOverride);
        ParsedType* baseType = field->varDecl.declaredTypes
            ? &field->varDecl.declaredTypes[0]
            : &field->varDecl.declaredType;
        size_t fieldSize = 0, fieldAlign = 0;
        if (!size_align_of_parsed_type(baseType, scope, &fieldSize, &fieldAlign)) return false;
        if (fieldPacked) fieldAlign = 1;
        if (fieldAlignOverride > 0 && fieldAlignOverride > fieldAlign) {
            fieldAlign = fieldAlignOverride;
        }
        if (fieldSize > maxSize) maxSize = fieldSize;
        if (fieldAlign > maxAlign) maxAlign = fieldAlign;
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
    if (kind == CC_TAG_STRUCT) {
        ok = layout_struct_fields(def, scope, &sz, &al);
    } else {
        ok = layout_union_fields(def, scope, &sz, &al);
    }
    if (ok) {
        cc_set_tag_layout(ctx, kind, name, sz, al);
        if (sizeOut) *sizeOut = sz;
        if (alignOut) *alignOut = al;
    }
    cc_tag_mark_computing(ctx, kind, name, false);
    return ok;
}
