// SPDX-License-Identifier: Apache-2.0

#include "Parser/Helpers/parsed_type.h"
#include "AST/ast_node.h"

#include <stdlib.h>
#include <string.h>

const char* getTokenTypeName(TokenType type) {
    switch (type) {
        case TOKEN_INT: return "int";
        case TOKEN_FLOAT: return "float";
        case TOKEN_DOUBLE: return "double";
        case TOKEN_CHAR: return "char";
        case TOKEN_BOOL: return "_Bool";
        case TOKEN_VOID: return "void";
        case TOKEN_SHORT: return "short";
        case TOKEN_LONG: return "long";
        case TOKEN_CONST: return "const";
        case TOKEN_SIGNED: return "signed";
        case TOKEN_UNSIGNED: return "unsigned";
        case TOKEN_VOLATILE: return "volatile";
        case TOKEN_INLINE: return "inline";
        case TOKEN_IDENTIFIER: return "identifier";
        default: return "unknown";
    }
}

void parsedTypeAddPointerDepth(ParsedType* t, int depth) {
    if (!t || depth <= 0) return;
    t->pointerDepth += depth;
}

static bool parsedTypeCloneInto(ParsedType* dst, const ParsedType* src);
static bool cloneDerivationInto(TypeDerivation* dst, const TypeDerivation* src);

static void parsedTypeFreeShallowArray(ParsedType* items, size_t count) {
    if (!items) return;
    for (size_t i = 0; i < count; ++i) {
        parsedTypeFree(&items[i]);
    }
    free(items);
}

static bool cloneParsedTypeArray(ParsedType** outItems,
                                 size_t* outCount,
                                 const ParsedType* srcItems,
                                 size_t count) {
    if (!outItems || !outCount) return false;
    *outItems = NULL;
    *outCount = 0;
    if (!srcItems || count == 0) {
        return true;
    }

    ParsedType* cloned = (ParsedType*)calloc(count, sizeof(ParsedType));
    if (!cloned) {
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        if (!parsedTypeCloneInto(&cloned[i], &srcItems[i])) {
            parsedTypeFreeShallowArray(cloned, i);
            return false;
        }
    }
    *outItems = cloned;
    *outCount = count;
    return true;
}

static void refreshFunctionFlags(ParsedType* t);
static const TypeDerivation* findFirstFunctionDerivation(const ParsedType* t);
static size_t countFunctionDerivations(const ParsedType* t);
static bool parsedTypeCloneInto(ParsedType* dst, const ParsedType* src);
static bool cloneDerivationInto(TypeDerivation* dst, const TypeDerivation* src);

static bool ensureDerivationCapacity(ParsedType* t, size_t extra) {
    if (!t) return false;
    size_t newCount = t->derivationCount + extra;
    TypeDerivation* newData = (TypeDerivation*)realloc(t->derivations, newCount * sizeof(TypeDerivation));
    if (!newData) {
        return false;
    }
    t->derivations = newData;
    return true;
}

void parsedTypeSetFunctionPointer(ParsedType* t, size_t nParams, const ParsedType* params) {
    if (!t) return;
    t->isFunctionPointer = true;
    parsedTypeFreeShallowArray(t->fpParams, t->fpParamCount);
    t->fpParams = NULL;
    t->fpParamCount = 0;

    /* Simple function-pointer declarators already encode their signature in a
       single function derivation. Avoid duplicating that graph into fpParams,
       because self-hosted clones can recurse through the redundant mirror.
       Nested callable shapes still need fpParams to preserve the innermost
       callable layer. */
    if (countFunctionDerivations(t) == 1 || nParams == 0) return;
    cloneParsedTypeArray(&t->fpParams, &t->fpParamCount, params, nParams);
}

void parsedTypeFree(ParsedType* t) {
    if (!t) return;
    if (t->fpParams) {
        parsedTypeFreeShallowArray(t->fpParams, t->fpParamCount);
        t->fpParams = NULL;
    }
    t->fpParamCount = 0;
    t->isFunctionPointer = false;
    parsedTypeResetDerivations(t);
    if (t->attributes) {
        astAttributeListDestroy(t->attributes, t->attributeCount);
        free(t->attributes);
        t->attributes = NULL;
    }
    t->attributeCount = 0;
}

bool parsedTypeAppendPointer(ParsedType* t) {
    if (!ensureDerivationCapacity(t, 1)) {
        return false;
    }
    TypeDerivation* slot = &t->derivations[t->derivationCount++];
    slot->kind = TYPE_DERIVATION_POINTER;
    slot->as.pointer.isConst = false;
    slot->as.pointer.isRestrict = false;
    slot->as.pointer.isVolatile = false;
    parsedTypeAddPointerDepth(t, 1);
    return true;
}

bool parsedTypeAppendArray(ParsedType* t, struct ASTNode* sizeExpr, bool isVLA) {
    if (!ensureDerivationCapacity(t, 1)) {
        return false;
    }
    TypeDerivation* slot = &t->derivations[t->derivationCount++];
    slot->kind = TYPE_DERIVATION_ARRAY;
    slot->as.array.sizeExpr = sizeExpr;
    slot->as.array.isVLA = isVLA;
    slot->as.array.hasConstantSize = false;
    slot->as.array.constantSize = 0;
    slot->as.array.isFlexible = false;
    slot->as.array.hasStatic = false;
    slot->as.array.qualConst = false;
    slot->as.array.qualVolatile = false;
    slot->as.array.qualRestrict = false;
    if (isVLA) {
        t->isVLA = true;
    }
    return true;
}

bool parsedTypeAppendFunction(ParsedType* t, const ParsedType* params, size_t paramCount, bool isVariadic) {
    if (!ensureDerivationCapacity(t, 1)) {
        return false;
    }
    TypeDerivation* slot = &t->derivations[t->derivationCount];
    slot->kind = TYPE_DERIVATION_FUNCTION;
    slot->as.function.isVariadic = isVariadic;
    slot->as.function.paramCount = paramCount;
    slot->as.function.params = NULL;
    if (paramCount > 0) {
        if (!cloneParsedTypeArray(&slot->as.function.params,
                                  &slot->as.function.paramCount,
                                  params,
                                  paramCount)) {
            slot->as.function.paramCount = 0;
            return false;
        }
    }
    t->derivationCount++;
    t->directlyDeclaresFunction = true;
    t->isVariadicFunction = isVariadic;
    return true;
}

void parsedTypeResetDerivations(ParsedType* t) {
    if (!t) return;
    if (t->derivations) {
        for (size_t i = 0; i < t->derivationCount; ++i) {
            TypeDerivation* slot = &t->derivations[i];
            if (slot->kind == TYPE_DERIVATION_FUNCTION && slot->as.function.params) {
                parsedTypeFreeShallowArray(slot->as.function.params, slot->as.function.paramCount);
                slot->as.function.params = NULL;
                slot->as.function.paramCount = 0;
            }
        }
        free(t->derivations);
        t->derivations = NULL;
    }
    t->derivationCount = 0;
    t->directlyDeclaresFunction = false;
    t->isVariadicFunction = false;
}

static bool cloneDerivationInto(TypeDerivation* dst, const TypeDerivation* src) {
    if (!dst) {
        return false;
    }
    memset(dst, 0, sizeof(*dst));
    if (!src) {
        return true;
    }
    dst->kind = src->kind;
    switch (src->kind) {
        case TYPE_DERIVATION_POINTER:
            dst->as.pointer = src->as.pointer;
            return true;
        case TYPE_DERIVATION_ARRAY:
            dst->as.array.sizeExpr = src->as.array.sizeExpr;
            dst->as.array.isVLA = src->as.array.isVLA;
            dst->as.array.hasConstantSize = src->as.array.hasConstantSize;
            dst->as.array.constantSize = src->as.array.constantSize;
            dst->as.array.isFlexible = src->as.array.isFlexible;
            dst->as.array.hasStatic = src->as.array.hasStatic;
            dst->as.array.qualConst = src->as.array.qualConst;
            dst->as.array.qualVolatile = src->as.array.qualVolatile;
            dst->as.array.qualRestrict = src->as.array.qualRestrict;
            return true;
        case TYPE_DERIVATION_FUNCTION:
            dst->as.function.isVariadic = src->as.function.isVariadic;
            dst->as.function.paramCount = 0;
            dst->as.function.params = NULL;
            return cloneParsedTypeArray(&dst->as.function.params,
                                        &dst->as.function.paramCount,
                                        src->as.function.params,
                                        src->as.function.paramCount);
    }
    return true;
}

static bool parsedTypeCloneInto(ParsedType* dst, const ParsedType* src) {
    if (!dst) {
        return false;
    }
    if (!src) {
        memset(dst, 0, sizeof(*dst));
        dst->kind = TYPE_INVALID;
        dst->tag = TAG_NONE;
        return true;
    }
    memset(dst, 0, sizeof(*dst));
    dst->kind = src->kind;
    dst->tag = src->tag;
    dst->inlineStructOrUnionDef = src->inlineStructOrUnionDef;
    dst->inlineEnumDef = src->inlineEnumDef;
    dst->isFunctionPointer = src->isFunctionPointer;
    dst->primitiveType = src->primitiveType;
    dst->userTypeName = src->userTypeName;
    dst->isConst = src->isConst;
    dst->isSigned = src->isSigned;
    dst->isUnsigned = src->isUnsigned;
    dst->isShort = src->isShort;
    dst->isLong = src->isLong;
    dst->isComplex = src->isComplex;
    dst->isImaginary = src->isImaginary;
    dst->isVolatile = src->isVolatile;
    dst->isRestrict = src->isRestrict;
    dst->isInline = src->isInline;
    dst->isStatic = src->isStatic;
    dst->isExtern = src->isExtern;
    dst->isRegister = src->isRegister;
    dst->isAuto = src->isAuto;
    dst->pointerDepth = src->pointerDepth;
    dst->isVLA = src->isVLA;
    dst->directlyDeclaresFunction = src->directlyDeclaresFunction;
    dst->isVariadicFunction = src->isVariadicFunction;
    dst->alignOverride = src->alignOverride;
    dst->hasAlignOverride = src->hasAlignOverride;
    dst->hasParamArrayInfo = src->hasParamArrayInfo;
    dst->paramArrayInfo = src->paramArrayInfo;
    if (src->fpParamCount > 0 &&
        src->fpParams &&
        findFirstFunctionDerivation(src) == NULL) {
        if (!cloneParsedTypeArray(&dst->fpParams, &dst->fpParamCount, src->fpParams, src->fpParamCount)) {
            return false;
        }
    }
    if (src->derivationCount > 0 && src->derivations) {
        dst->derivationCount = src->derivationCount;
        dst->derivations = (TypeDerivation*)calloc(src->derivationCount, sizeof(TypeDerivation));
        if (dst->derivations) {
            for (size_t i = 0; i < src->derivationCount; ++i) {
                if (!cloneDerivationInto(&dst->derivations[i], &src->derivations[i])) {
                    dst->derivationCount = i;
                    parsedTypeResetDerivations(dst);
                    return false;
                }
            }
        } else {
            dst->derivationCount = 0;
        }
    } else {
        dst->derivationCount = 0;
    }
    if (src->attributeCount > 0 && src->attributes) {
        dst->attributes = astAttributeListClone(src->attributes, src->attributeCount);
        if (dst->attributes) {
            dst->attributeCount = src->attributeCount;
        }
    }
    return true;
}

ParsedType parsedTypeClone(const ParsedType* src) {
    ParsedType copy;
    if (!parsedTypeCloneInto(&copy, src)) {
        memset(&copy, 0, sizeof(copy));
        copy.kind = TYPE_INVALID;
        copy.tag = TAG_NONE;
    }
    return copy;
}

void parsedTypeAdoptAttributes(ParsedType* t, ASTAttribute** attrs, size_t count) {
    if (!t) {
        astAttributeListDestroy(attrs, count);
        free(attrs);
        return;
    }
    astAttributeListAppend(&t->attributes, &t->attributeCount, attrs, count);
}

bool parsedTypeIsDirectArray(const ParsedType* t) {
    return t && t->derivationCount > 0 && t->derivations[0].kind == TYPE_DERIVATION_ARRAY;
}

bool parsedTypeAdjustArrayParameter(ParsedType* t) {
    if (!parsedTypeIsDirectArray(t)) {
        return false;
    }
    TypeDerivation* arr = &t->derivations[0];
    t->hasParamArrayInfo = true;
    t->paramArrayInfo = arr->as.array;

    TypeDerivation ptr;
    memset(&ptr, 0, sizeof(ptr));
    ptr.kind = TYPE_DERIVATION_POINTER;
    ptr.as.pointer.isConst = arr->as.array.qualConst;
    ptr.as.pointer.isVolatile = arr->as.array.qualVolatile;
    ptr.as.pointer.isRestrict = arr->as.array.qualRestrict;
    t->derivations[0] = ptr;
    parsedTypeAddPointerDepth(t, 1);
    t->isVLA = parsedTypeHasVLA(t);
    return true;
}

ParsedType parsedTypeArrayElementType(const ParsedType* t) {
    ParsedType element = parsedTypeClone(t);
    if (!parsedTypeIsDirectArray(&element)) {
        return element;
    }
    if (element.derivationCount > 1) {
        memmove(element.derivations,
                element.derivations + 1,
                (element.derivationCount - 1) * sizeof(TypeDerivation));
    }
    element.derivationCount--;
    if (element.derivationCount == 0 && element.derivations) {
        free(element.derivations);
        element.derivations = NULL;
    }
    refreshFunctionFlags(&element);
    return element;
}

ParsedType parsedTypePointerTargetType(const ParsedType* t) {
    ParsedType copy = parsedTypeClone(t);
    if (!t) {
        copy.kind = TYPE_INVALID;
        return copy;
    }
    bool removed = false;
    for (size_t i = 0; i < copy.derivationCount; ++i) {
        if (copy.derivations[i].kind != TYPE_DERIVATION_POINTER) {
            continue;
        }
        if (i + 1 < copy.derivationCount) {
            memmove(copy.derivations + i,
                    copy.derivations + i + 1,
                    (copy.derivationCount - i - 1) * sizeof(TypeDerivation));
        }
        copy.derivationCount--;
        if (copy.pointerDepth > 0) {
            copy.pointerDepth -= 1;
        }
        removed = true;
        break;
    }
    if (!removed && copy.pointerDepth > 0) {
        copy.pointerDepth -= 1;
        removed = true;
    }
    if (!removed) {
        parsedTypeFree(&copy);
        memset(&copy, 0, sizeof(copy));
        copy.kind = TYPE_INVALID;
        copy.tag = TAG_NONE;
        return copy;
    }
    refreshFunctionFlags(&copy);
    return copy;
}

ParsedType parsedTypeFunctionReturnType(const ParsedType* t) {
    ParsedType copy = parsedTypeClone(t);
    if (!t) {
        copy.kind = TYPE_INVALID;
        return copy;
    }
    if (copy.derivationCount == 0) {
        copy.kind = TYPE_INVALID;
        return copy;
    }
    size_t funcIndex = (size_t)-1;
    size_t removedPointers = 0;
    for (size_t i = 0; i < copy.derivationCount; ++i) {
        if (copy.derivations[i].kind == TYPE_DERIVATION_POINTER) {
            removedPointers++;
        }
        if (copy.derivations[i].kind == TYPE_DERIVATION_FUNCTION) {
            funcIndex = i;
            break;
        }
    }
    if (funcIndex == (size_t)-1) {
        copy.kind = TYPE_INVALID;
        return copy;
    }
    if (funcIndex + 1 < copy.derivationCount) {
        memmove(copy.derivations,
                copy.derivations + funcIndex + 1,
                (copy.derivationCount - funcIndex - 1) * sizeof(TypeDerivation));
    }
    copy.derivationCount -= (funcIndex + 1);
    if (copy.derivationCount == 0 && copy.derivations) {
        free(copy.derivations);
        copy.derivations = NULL;
    }
    if (removedPointers > 0 && copy.pointerDepth > 0) {
        int dec = (int)removedPointers;
        copy.pointerDepth = copy.pointerDepth > dec ? (copy.pointerDepth - dec) : 0;
    }
    refreshFunctionFlags(&copy);
    return copy;
}

ParsedType parsedTypeDeclaredFunctionReturnType(const ParsedType* t) {
    ParsedType copy = parsedTypeClone(t);
    if (!t) {
        copy.kind = TYPE_INVALID;
        return copy;
    }
    if (copy.derivationCount == 0) {
        copy.kind = TYPE_INVALID;
        return copy;
    }

    size_t funcIndex = (size_t)-1;
    for (size_t i = 0; i < copy.derivationCount; ++i) {
        if (copy.derivations[i].kind == TYPE_DERIVATION_FUNCTION) {
            funcIndex = i;
            break;
        }
    }
    if (funcIndex == (size_t)-1) {
        copy.kind = TYPE_INVALID;
        return copy;
    }

    if (funcIndex + 1 < copy.derivationCount) {
        memmove(copy.derivations + funcIndex,
                copy.derivations + funcIndex + 1,
                (copy.derivationCount - funcIndex - 1) * sizeof(TypeDerivation));
    }
    copy.derivationCount--;
    if (copy.derivationCount == 0 && copy.derivations) {
        free(copy.derivations);
        copy.derivations = NULL;
    }

    size_t remainingFuncIndex = (size_t)-1;
    size_t leadingPointerCount = 0;
    for (size_t i = 0; i < copy.derivationCount; ++i) {
        if (copy.derivations[i].kind == TYPE_DERIVATION_FUNCTION) {
            remainingFuncIndex = i;
            break;
        }
        if (copy.derivations[i].kind == TYPE_DERIVATION_POINTER) {
            leadingPointerCount++;
        }
    }

    if (remainingFuncIndex != (size_t)-1 && leadingPointerCount > 1) {
        TypeDerivation* reordered =
            (TypeDerivation*)malloc(copy.derivationCount * sizeof(TypeDerivation));
        if (reordered) {
            size_t out = 0;
            size_t keptPointers = 0;
            size_t movedCount = 0;

            for (size_t i = 0; i < remainingFuncIndex; ++i) {
                if (copy.derivations[i].kind == TYPE_DERIVATION_POINTER) {
                    if (keptPointers == 0) {
                        reordered[out++] = copy.derivations[i];
                    } else {
                        movedCount++;
                    }
                    keptPointers++;
                } else {
                    reordered[out++] = copy.derivations[i];
                }
            }

            reordered[out++] = copy.derivations[remainingFuncIndex];

            if (movedCount > 0) {
                size_t emittedMoves = 0;
                size_t seenPointers = 0;
                for (size_t i = 0; i < remainingFuncIndex; ++i) {
                    if (copy.derivations[i].kind != TYPE_DERIVATION_POINTER) {
                        continue;
                    }
                    if (seenPointers++ == 0) {
                        continue;
                    }
                    reordered[out++] = copy.derivations[i];
                    emittedMoves++;
                }
                (void)emittedMoves;
            }

            for (size_t i = remainingFuncIndex + 1; i < copy.derivationCount; ++i) {
                reordered[out++] = copy.derivations[i];
            }

            if (out == copy.derivationCount) {
                memcpy(copy.derivations,
                       reordered,
                       copy.derivationCount * sizeof(TypeDerivation));
            }
            free(reordered);
        }
    }

    refreshFunctionFlags(&copy);
    return copy;
}

void parsedTypeNormalizeFunctionPointer(ParsedType* t) {
    if (!t || !t->isFunctionPointer || t->derivationCount < 2 || !t->derivations) {
        return;
    }
    size_t funcIndex = (size_t)-1;
    for (size_t i = 0; i < t->derivationCount; ++i) {
        if (t->derivations[i].kind == TYPE_DERIVATION_FUNCTION) {
            funcIndex = i;
            break;
        }
    }
    if (funcIndex == (size_t)-1) {
        return;
    }

    for (size_t i = 0; i < funcIndex; ++i) {
        if (t->derivations[i].kind == TYPE_DERIVATION_POINTER) {
            return;
        }
    }

    size_t pointerAfterCount = 0;
    for (size_t i = funcIndex + 1; i < t->derivationCount; ++i) {
        if (t->derivations[i].kind == TYPE_DERIVATION_POINTER) {
            pointerAfterCount++;
        }
    }
    if (pointerAfterCount == 0) {
        return;
    }

    TypeDerivation* reordered = (TypeDerivation*)malloc(t->derivationCount * sizeof(TypeDerivation));
    if (!reordered) {
        return;
    }

    size_t out = 0;
    for (size_t i = 0; i < funcIndex; ++i) {
        reordered[out++] = t->derivations[i];
    }
    for (size_t i = funcIndex + 1; i < t->derivationCount; ++i) {
        if (t->derivations[i].kind == TYPE_DERIVATION_POINTER) {
            reordered[out++] = t->derivations[i];
        }
    }
    reordered[out++] = t->derivations[funcIndex];
    for (size_t i = funcIndex + 1; i < t->derivationCount; ++i) {
        if (t->derivations[i].kind != TYPE_DERIVATION_POINTER) {
            reordered[out++] = t->derivations[i];
        }
    }

    memcpy(t->derivations, reordered, t->derivationCount * sizeof(TypeDerivation));
    free(reordered);
}

static const TypeDerivation* findFirstFunctionDerivation(const ParsedType* t) {
    if (!t || !t->derivations) {
        return NULL;
    }
    for (size_t i = 0; i < t->derivationCount; ++i) {
        if (t->derivations[i].kind == TYPE_DERIVATION_FUNCTION) {
            return &t->derivations[i];
        }
    }
    return NULL;
}

static size_t countFunctionDerivations(const ParsedType* t) {
    if (!t || !t->derivations) {
        return 0;
    }
    size_t count = 0;
    for (size_t i = 0; i < t->derivationCount; ++i) {
        if (t->derivations[i].kind == TYPE_DERIVATION_FUNCTION) {
            count++;
        }
    }
    return count;
}


bool parsedTypeGetEffectiveFunctionPointerSignature(const ParsedType* t,
                                                    const ParsedType** params,
                                                    size_t* count,
                                                    bool* isVariadic) {
    if (params) *params = NULL;
    if (count) *count = 0;
    if (isVariadic) *isVariadic = false;
    if (!t || !t->isFunctionPointer) {
        return false;
    }

    if (t->fpParams || t->fpParamCount > 0) {
        if (params) *params = t->fpParams;
        if (count) *count = t->fpParamCount;
        return true;
    }

    const TypeDerivation* func = findFirstFunctionDerivation(t);
    if (func) {
        if (params) *params = func->as.function.params;
        if (count) *count = func->as.function.paramCount;
        if (isVariadic) *isVariadic = func->as.function.isVariadic;
        return true;
    }

    return false;
}

static bool namesEqual(const char* a, const char* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

static void refreshFunctionFlags(ParsedType* t) {
    if (!t) return;
    t->isFunctionPointer = false;
    t->directlyDeclaresFunction = false;
    t->isVariadicFunction = false;

    size_t funcIndex = (size_t)-1;
    for (size_t i = 0; i < t->derivationCount; ++i) {
        if (t->derivations[i].kind != TYPE_DERIVATION_FUNCTION) {
            continue;
        }
        funcIndex = i;
        t->directlyDeclaresFunction = true;
        t->isVariadicFunction = t->derivations[i].as.function.isVariadic;
        break;
    }
    if (funcIndex == (size_t)-1) {
        return;
    }
    for (size_t i = 0; i < funcIndex; ++i) {
        if (t->derivations[i].kind == TYPE_DERIVATION_POINTER) {
            t->isFunctionPointer = true;
            return;
        }
    }
}

bool parsedTypesStructurallyEqual(const ParsedType* a, const ParsedType* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    if (a->primitiveType != b->primitiveType) return false;
    if (a->tag != b->tag) return false;
    if (!namesEqual(a->userTypeName, b->userTypeName)) return false;
    if (a->isConst != b->isConst) return false;
    if (a->isVolatile != b->isVolatile) return false;
    if (a->isRestrict != b->isRestrict) return false;
    if (a->isSigned != b->isSigned) return false;
    if (a->isUnsigned != b->isUnsigned) return false;
    if (a->isShort != b->isShort) return false;
    if (a->isLong != b->isLong) return false;
    if (a->isComplex != b->isComplex) return false;
    if (a->isImaginary != b->isImaginary) return false;
    if (a->pointerDepth != b->pointerDepth) return false;
    if (a->isFunctionPointer || b->isFunctionPointer) {
        const ParsedType* lhsParams = NULL;
        const ParsedType* rhsParams = NULL;
        size_t lhsCount = 0;
        size_t rhsCount = 0;
        bool lhsVariadic = false;
        bool rhsVariadic = false;
        bool lhsHave = parsedTypeGetEffectiveFunctionPointerSignature(a, &lhsParams, &lhsCount, &lhsVariadic);
        bool rhsHave = parsedTypeGetEffectiveFunctionPointerSignature(b, &rhsParams, &rhsCount, &rhsVariadic);
        if (lhsHave != rhsHave) return false;
        if (lhsHave) {
            if (lhsCount != rhsCount) return false;
            if (lhsVariadic != rhsVariadic) return false;
            for (size_t i = 0; i < lhsCount; ++i) {
                if (!parsedTypesStructurallyEqual(&lhsParams[i], &rhsParams[i])) {
                    return false;
                }
            }
        } else if (a->fpParamCount != b->fpParamCount) {
            return false;
        }
        for (size_t i = 0; i < a->fpParamCount; ++i) {
            if (!parsedTypesStructurallyEqual(&a->fpParams[i], &b->fpParams[i])) {
                return false;
            }
        }
    }
    if (a->derivationCount != b->derivationCount) return false;

    for (size_t i = 0; i < a->derivationCount; ++i) {
        const TypeDerivation* lhs = &a->derivations[i];
        const TypeDerivation* rhs = &b->derivations[i];
        if (lhs->kind != rhs->kind) {
            return false;
        }
        switch (lhs->kind) {
            case TYPE_DERIVATION_POINTER:
                if (lhs->as.pointer.isConst != rhs->as.pointer.isConst) return false;
                if (lhs->as.pointer.isVolatile != rhs->as.pointer.isVolatile) return false;
                if (lhs->as.pointer.isRestrict != rhs->as.pointer.isRestrict) return false;
                break;
            case TYPE_DERIVATION_ARRAY:
                if (lhs->as.array.isVLA != rhs->as.array.isVLA) return false;
                break;
            case TYPE_DERIVATION_FUNCTION:
                if (lhs->as.function.isVariadic != rhs->as.function.isVariadic) return false;
                if (lhs->as.function.paramCount != rhs->as.function.paramCount) return false;
                for (size_t p = 0; p < lhs->as.function.paramCount; ++p) {
                    if (!parsedTypesStructurallyEqual(&lhs->as.function.params[p],
                                                      &rhs->as.function.params[p])) {
                        return false;
                    }
                }
                break;
        }
    }
    return true;
}

const TypeDerivation* parsedTypeGetDerivation(const ParsedType* t, size_t index) {
    if (!t || index >= t->derivationCount) {
        return NULL;
    }
    return &t->derivations[index];
}

const TypeDerivation* parsedTypeGetArrayDerivation(const ParsedType* t, size_t dimensionIndex) {
    if (!t) {
        return NULL;
    }
    size_t seen = 0;
    for (size_t i = 0; i < t->derivationCount; ++i) {
        const TypeDerivation* deriv = &t->derivations[i];
        if (deriv->kind != TYPE_DERIVATION_ARRAY) {
            continue;
        }
        if (seen == dimensionIndex) {
            return deriv;
        }
        seen++;
    }
    return NULL;
}

TypeDerivation* parsedTypeGetMutableArrayDerivation(ParsedType* t, size_t dimensionIndex) {
    if (!t) {
        return NULL;
    }
    size_t seen = 0;
    for (size_t i = 0; i < t->derivationCount; ++i) {
        TypeDerivation* deriv = &t->derivations[i];
        if (deriv->kind != TYPE_DERIVATION_ARRAY) {
            continue;
        }
        if (seen == dimensionIndex) {
            return deriv;
        }
        seen++;
    }
    return NULL;
}

size_t parsedTypeCountDerivationsOfKind(const ParsedType* t, TypeDerivationKind kind) {
    if (!t) {
        return 0;
    }
    size_t count = 0;
    for (size_t i = 0; i < t->derivationCount; ++i) {
        if (t->derivations[i].kind == kind) {
            count++;
        }
    }
    return count;
}

bool parsedTypeHasVLA(const ParsedType* t) {
    if (!t) {
        return false;
    }
    for (size_t i = 0; i < t->derivationCount; ++i) {
        if (t->derivations[i].kind != TYPE_DERIVATION_ARRAY) {
            continue;
        }
        const TypeDerivation* arr = &t->derivations[i];
        if (arr->as.array.isVLA) {
            return true;
        }
        if (!arr->as.array.hasConstantSize &&
            arr->as.array.sizeExpr &&
            !arr->as.array.isFlexible) {
            ASTNodeType exprTy = arr->as.array.sizeExpr->type;
            if (exprTy != AST_NUMBER_LITERAL && exprTy != AST_CHAR_LITERAL) {
                return true;
            }
        }
    }
    return false;
}
