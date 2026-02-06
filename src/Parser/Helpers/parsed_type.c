#include "Parser/Helpers/parsed_type.h"
#include "Parser/parser.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Compiler/compiler_context.h"
#include "Parser/Helpers/parser_attributes.h"
#include "Parser/parser_decl.h"
#include "Parser/Expr/parser_expr.h"
#include "AST/ast_node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Convert TokenType to string for debugging and printing
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
    t->fpParamCount = 0;
    t->fpParams = NULL;

    if (nParams == 0) return;

    ParsedType* copy = (ParsedType*)malloc(nParams * sizeof(ParsedType));
    if (!copy) return;

    memcpy(copy, params, nParams * sizeof(ParsedType));
    t->fpParams = copy;
    t->fpParamCount = nParams;
}

void parsedTypeFree(ParsedType* t) {
    if (!t) return;
    if (t->fpParams) {
        free(t->fpParams);
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

static ParsedType parsedTypeDefault(void) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_INVALID;
    t.tag = TAG_NONE;
    t.primitiveType = TOKEN_VOID;
    t.alignOverride = 0;
    t.hasAlignOverride = false;
    return t;
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
        size_t allocCount = paramCount * sizeof(ParsedType);
        slot->as.function.params = (ParsedType*)malloc(allocCount);
        if (!slot->as.function.params) {
            slot->as.function.paramCount = 0;
            return false;
        }
        memcpy(slot->as.function.params, params, allocCount);
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
                free(slot->as.function.params);
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

static TypeDerivation cloneDerivation(const TypeDerivation* src) {
    TypeDerivation dst;
    memset(&dst, 0, sizeof(dst));
    if (!src) {
        return dst;
    }
    dst.kind = src->kind;
    switch (src->kind) {
        case TYPE_DERIVATION_POINTER:
            dst.as.pointer = src->as.pointer;
            break;
        case TYPE_DERIVATION_ARRAY:
            dst.as.array.sizeExpr = src->as.array.sizeExpr;
            dst.as.array.isVLA = src->as.array.isVLA;
            dst.as.array.hasConstantSize = src->as.array.hasConstantSize;
            dst.as.array.constantSize = src->as.array.constantSize;
            dst.as.array.isFlexible = src->as.array.isFlexible;
            dst.as.array.hasStatic = src->as.array.hasStatic;
            dst.as.array.qualConst = src->as.array.qualConst;
            dst.as.array.qualVolatile = src->as.array.qualVolatile;
            dst.as.array.qualRestrict = src->as.array.qualRestrict;
            break;
        case TYPE_DERIVATION_FUNCTION:
            dst.as.function.isVariadic = src->as.function.isVariadic;
            dst.as.function.paramCount = src->as.function.paramCount;
            dst.as.function.params = NULL;
            if (src->as.function.params && src->as.function.paramCount > 0) {
                size_t bytes = src->as.function.paramCount * sizeof(ParsedType);
                dst.as.function.params = (ParsedType*)malloc(bytes);
                if (dst.as.function.params) {
                    memcpy(dst.as.function.params, src->as.function.params, bytes);
                } else {
                    dst.as.function.paramCount = 0;
                }
            }
            break;
    }
    return dst;
}

ParsedType parsedTypeClone(const ParsedType* src) {
    ParsedType copy;
    if (!src) {
        memset(&copy, 0, sizeof(copy));
        copy.kind = TYPE_INVALID;
        copy.tag = TAG_NONE;
        return copy;
    }
    memcpy(&copy, src, sizeof(copy));
    copy.fpParams = NULL;
    copy.derivations = NULL;
    copy.fpParamCount = 0;
    copy.attributes = NULL;
    copy.attributeCount = 0;
    if (src->fpParamCount > 0 && src->fpParams) {
        size_t bytes = src->fpParamCount * sizeof(ParsedType);
        copy.fpParams = (ParsedType*)malloc(bytes);
        if (copy.fpParams) {
            memcpy(copy.fpParams, src->fpParams, bytes);
            copy.fpParamCount = src->fpParamCount;
        }
    }
    if (src->derivationCount > 0 && src->derivations) {
        copy.derivationCount = src->derivationCount;
        copy.derivations = (TypeDerivation*)malloc(src->derivationCount * sizeof(TypeDerivation));
        if (copy.derivations) {
            for (size_t i = 0; i < src->derivationCount; ++i) {
                copy.derivations[i] = cloneDerivation(&src->derivations[i]);
            }
        } else {
            copy.derivationCount = 0;
        }
    } else {
        copy.derivationCount = 0;
    }
    if (src->attributeCount > 0 && src->attributes) {
        copy.attributes = astAttributeListClone(src->attributes, src->attributeCount);
        if (copy.attributes) {
            copy.attributeCount = src->attributeCount;
        }
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
    }
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
    if (t->isFunctionPointer && copy.pointerDepth > 0) {
        copy.pointerDepth -= 1;
    }
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

static bool namesEqual(const char* a, const char* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

bool parsedTypesStructurallyEqual(const ParsedType* a, const ParsedType* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    if (a->primitiveType != b->primitiveType) return false;
    if (a->tag != b->tag) return false;
    if (!namesEqual(a->userTypeName, b->userTypeName)) return false;
    if (a->pointerDepth != b->pointerDepth) return false;
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
        if (t->derivations[i].kind == TYPE_DERIVATION_ARRAY &&
            t->derivations[i].as.array.isVLA) {
            return true;
        }
    }
    return false;
}

static bool identifierMatchesKnownType(Parser* parser, const char* name) {
    if (!parser || !parser->ctx || !name) {
        return false;
    }
    if (cc_is_typedef(parser->ctx, name)) {
        return true;
    }
    if (cc_is_builtin_type(parser->ctx, name)) {
        return true;
    }
    return false;
}

static TagKind tagKindFromToken(TokenType tok) {
    switch (tok) {
        case TOKEN_STRUCT: return TAG_STRUCT;
        case TOKEN_UNION:  return TAG_UNION;
        case TOKEN_ENUM:   return TAG_ENUM;
        default:           return TAG_NONE;
    }
}

static bool tagIsKnown(Parser* parser, TagKind tag, const char* name) {
    if (!parser || !parser->ctx || tag == TAG_NONE || !name) {
        return false;
    }
    switch (tag) {
        case TAG_STRUCT:
            return cc_has_tag(parser->ctx, CC_TAG_STRUCT, name);
        case TAG_UNION:
            return cc_has_tag(parser->ctx, CC_TAG_UNION, name);
        case TAG_ENUM:
            return cc_has_tag(parser->ctx, CC_TAG_ENUM, name);
        case TAG_NONE:
        default:
            return false;
    }
}

typedef struct PointerQuals {
    bool isConst;
    bool isVolatile;
    bool isRestrict;
} PointerQuals;

static PointerQuals consumePointerQualifiers(Parser* parser) {
    PointerQuals quals = {0};
    while (parser->currentToken.type == TOKEN_CONST ||
           parser->currentToken.type == TOKEN_VOLATILE ||
           parser->currentToken.type == TOKEN_RESTRICT) {
        if (parser->currentToken.type == TOKEN_CONST) {
            quals.isConst = true;
        } else if (parser->currentToken.type == TOKEN_VOLATILE) {
            quals.isVolatile = true;
        } else if (parser->currentToken.type == TOKEN_RESTRICT) {
            quals.isRestrict = true;
        }
        advance(parser);
    }
    return quals;
}

static void consumeTypeAttributes(Parser* parser, ParsedType* type) {
    if (!parser || !type) return;
    size_t attrCount = 0;
    ASTAttribute** attrs = parserParseAttributeSpecifiers(parser, &attrCount);
    parsedTypeAdoptAttributes(type, attrs, attrCount);
}

static bool isAlignasToken(const Token* tok) {
    if (!tok || tok->type != TOKEN_IDENTIFIER || !tok->value) return false;
    return strcmp(tok->value, "_Alignas") == 0 || strcmp(tok->value, "alignas") == 0;
}

static void consumeAlignasSpecifiers(Parser* parser, ParsedType* type) {
    if (!parser || !type) return;
    while (isAlignasToken(&parser->currentToken)) {
        advance(parser); /* consume keyword */
        if (parser->currentToken.type != TOKEN_LPAREN) {
            printParseError("Expected '(' after alignas", parser);
            return;
        }
        advance(parser); /* consume '(' */
        ASTNode* expr = parseAssignmentExpression(parser);
        if (!expr) {
            printParseError("Invalid alignas argument", parser);
            return;
        }
        if (parser->currentToken.type != TOKEN_RPAREN) {
            printParseError("Expected ')' to close alignas", parser);
            return;
        }
        advance(parser); /* consume ')' */
        if (expr->type == AST_NUMBER_LITERAL && expr->valueNode.value) {
            char* endp = NULL;
            long long val = strtoll(expr->valueNode.value, &endp, 0);
            if (endp != expr->valueNode.value && val > 0) {
                size_t asSize = (size_t)val;
                if (!type->hasAlignOverride || asSize > type->alignOverride) {
                    type->alignOverride = asSize;
                }
                type->hasAlignOverride = true;
            }
        }
    }
}

static bool parseInlineEnumBody(Parser* parser,
                                ASTNode*** outMembers,
                                ASTNode*** outValues,
                                size_t* outCount) {
    if (!parser || !outMembers || !outValues || !outCount) return false;
    if (parser->currentToken.type != TOKEN_LBRACE) {
        printParseError("Expected '{' to begin enum body", parser);
        return false;
    }

    advance(parser); // consume '{'

    size_t count = 0;
    size_t capacity = 4;
    ASTNode** members = (ASTNode**)malloc(capacity * sizeof(ASTNode*));
    ASTNode** values = (ASTNode**)malloc(capacity * sizeof(ASTNode*));
    if (!members || !values) {
        free(members);
        free(values);
        return false;
    }

    while (parser->currentToken.type == TOKEN_IDENTIFIER) {
        ASTNode* memberName = createIdentifierNode(parser->currentToken.value);
        astNodeSetProvenance(memberName, &parser->currentToken);
        advance(parser); // consume member name

        ASTNode* valueExpr = NULL;
        if (parser->currentToken.type == TOKEN_ASSIGN) {
            advance(parser); // consume '='
            valueExpr = parseAssignmentExpression(parser);
            if (!valueExpr) {
                printParseError("Invalid value expression in enum", parser);
                free(members);
                free(values);
                return false;
            }
        }

        if (count >= capacity) {
            capacity *= 2;
            members = (ASTNode**)realloc(members, capacity * sizeof(ASTNode*));
            values = (ASTNode**)realloc(values, capacity * sizeof(ASTNode*));
            if (!members || !values) {
                return false;
            }
        }

        members[count] = memberName;
        values[count] = valueExpr;
        count++;

        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser); // continue
            if (parser->currentToken.type == TOKEN_RBRACE) {
                break;
            }
        } else if (parser->currentToken.type == TOKEN_RBRACE) {
            break;
        } else {
            printParseError("Expected ',' or '}' after enum member", parser);
            free(members);
            free(values);
            return false;
        }
    }

    if (parser->currentToken.type != TOKEN_RBRACE) {
        printParseError("Expected '}' to close enum body", parser);
        free(members);
        free(values);
        return false;
    }
    advance(parser); // consume '}'

    *outMembers = members;
    *outValues = values;
    *outCount = count;
    return true;
}

static ParsedType parseTypeCore(Parser* parser, TypeContext ctx) {
    ParsedType type = parsedTypeDefault();
    if (!parser) {
        return type;
    }
    consumeTypeAttributes(parser, &type);
    consumeAlignasSpecifiers(parser, &type);

    // Storage class specifiers
    for (;;) {
        switch (parser->currentToken.type) {
            case TOKEN_TYPEDEF:  advance(parser); continue;
            case TOKEN_STATIC:   type.isStatic   = true; advance(parser); continue;
            case TOKEN_EXTERN:   type.isExtern   = true; advance(parser); continue;
            case TOKEN_REGISTER: type.isRegister = true; advance(parser); continue;
            case TOKEN_AUTO:     type.isAuto     = true; advance(parser); continue;
            default: break;
        }
        consumeTypeAttributes(parser, &type);
        consumeAlignasSpecifiers(parser, &type);
        consumeAlignasSpecifiers(parser, &type);
        break;
    }

    // Type qualifiers / modifiers
    for (;;) {
        switch (parser->currentToken.type) {
            case TOKEN_CONST:    type.isConst    = true; advance(parser); continue;
            case TOKEN_SIGNED:   type.isSigned   = true; advance(parser); continue;
            case TOKEN_UNSIGNED: type.isUnsigned = true; advance(parser); continue;
            case TOKEN_SHORT:    type.isShort    = true; advance(parser); continue;
            case TOKEN_LONG:     type.isLong     = true; advance(parser); continue;
            case TOKEN_VOLATILE: type.isVolatile = true; advance(parser); continue;
            case TOKEN_RESTRICT: type.isRestrict = true; advance(parser); continue;
            case TOKEN_INLINE:   type.isInline   = true; advance(parser); continue;
            case TOKEN_COMPLEX:  type.isComplex  = true; advance(parser); continue;
            case TOKEN_IMAGINARY: type.isImaginary = true; advance(parser); continue;
            default: break;
        }
        consumeTypeAttributes(parser, &type);
        consumeAlignasSpecifiers(parser, &type);
        consumeAlignasSpecifiers(parser, &type);
        break;
    }

    bool sawBaseType = false;

    if (isPrimitiveTypeToken(parser->currentToken.type)) {
        type.kind = TYPE_PRIMITIVE;
        type.primitiveType = parser->currentToken.type;
        advance(parser);
        sawBaseType = true;

        /* Handle chains like 'long int' or 'unsigned long' by
           consuming any additional primitive specifiers that follow.
           We don't currently differentiate the combined form, but
           skipping the extra tokens keeps the parser in sync. */
        while (isPrimitiveTypeToken(parser->currentToken.type)) {
            advance(parser);
        }
    } else if (parser->currentToken.type == TOKEN_STRUCT ||
               parser->currentToken.type == TOKEN_UNION ||
               parser->currentToken.type == TOKEN_ENUM) {
        TokenType tagToken = parser->currentToken.type;
        TagKind tag = tagKindFromToken(tagToken);
        advance(parser); // consume keyword

        consumeTypeAttributes(parser, &type);

        char* tagName = NULL;
        int tagLine = parser->currentToken.line;
        if (parser->currentToken.type == TOKEN_IDENTIFIER) {
            tagName = strdup(parser->currentToken.value);
            advance(parser);
        }

        consumeTypeAttributes(parser, &type);

        bool hasBody = parser->currentToken.type == TOKEN_LBRACE &&
                       (tagToken == TOKEN_STRUCT || tagToken == TOKEN_UNION);
        bool hasEnumBody = parser->currentToken.type == TOKEN_LBRACE &&
                           tagToken == TOKEN_ENUM;
        if (hasBody) {
            advance(parser); // consume '{'
            size_t fieldCount = 0;
            bool hasFlexible = false;
            ASTNode** fields = parseStructOrUnionFields(parser, &fieldCount, &hasFlexible);
            if (parser->currentToken.type != TOKEN_RBRACE) {
                printParseError("Expected '}' to close struct/union body", parser);
                return parsedTypeDefault();
            }
            advance(parser); // consume '}'

            size_t trailingAttrCount = 0;
            ASTAttribute** trailingAttrs = parserParseAttributeSpecifiers(parser, &trailingAttrCount);
            if (type.hasAlignOverride && type.alignOverride > 0) {
                char buf[32];
                snprintf(buf, sizeof(buf), "aligned(%zu)", type.alignOverride);
                ASTAttribute* alignAttr = astAttributeCreate(AST_ATTRIBUTE_SYNTAX_GNU, buf);
                if (alignAttr) {
                    ASTAttribute** list = (ASTAttribute**)malloc(sizeof(ASTAttribute*));
                    if (list) {
                        list[0] = alignAttr;
                        astAttributeListAppend(&trailingAttrs, &trailingAttrCount, list, 1);
                    } else {
                        astAttributeDestroy(alignAttr);
                    }
                }
            }
            if (type.hasAlignOverride && type.alignOverride > 0) {
                char buf[32];
                snprintf(buf, sizeof(buf), "aligned(%zu)", type.alignOverride);
                ASTAttribute* alignAttr = astAttributeCreate(AST_ATTRIBUTE_SYNTAX_GNU, buf);
                if (alignAttr) {
                    ASTAttribute** list = (ASTAttribute**)malloc(sizeof(ASTAttribute*));
                    if (list) {
                        list[0] = alignAttr;
                        astAttributeListAppend(&trailingAttrs, &trailingAttrCount, list, 1);
                    } else {
                        astAttributeDestroy(alignAttr);
                    }
                }
            }

            if (!tagName) {
                char buffer[64];
                unsigned long long anonId = parser ? parser->anonTagCounter++ : 0ULL;
                snprintf(buffer, sizeof(buffer), "__anon_%s_%llu_%d",
                         tagToken == TOKEN_STRUCT ? "struct" : "union",
                         anonId,
                         parser->currentToken.line);
                tagName = strdup(buffer);
            }

            ASTNodeType defType = (tagToken == TOKEN_STRUCT)
                                   ? AST_STRUCT_DEFINITION
                                   : AST_UNION_DEFINITION;
            ASTNode* def = createStructOrUnionDefinitionNode(defType,
                                                             tagName ? tagName : "",
                                                             fields,
                                                             fieldCount);
            if (def) {
                def->line = tagLine;
                if (def->structDef.structName) def->structDef.structName->line = tagLine;
                def->structDef.hasFlexibleArray = hasFlexible;
                astNodeCloneTypeAttributes(def, &type);
                astNodeAppendAttributes(def, trailingAttrs, trailingAttrCount);
                trailingAttrs = NULL;
                trailingAttrCount = 0;
            } else {
                astAttributeListDestroy(trailingAttrs, trailingAttrCount);
                free(trailingAttrs);
            }

            type.tag = tag;
            type.kind = (tagToken == TOKEN_STRUCT) ? TYPE_STRUCT : TYPE_UNION;
            type.userTypeName = tagName ? strdup(tagName) : NULL;
            type.inlineStructOrUnionDef = def;
            sawBaseType = true;
            if (parser->ctx && tagName) {
                cc_add_tag(parser->ctx,
                           tagToken == TOKEN_STRUCT ? CC_TAG_STRUCT : CC_TAG_UNION,
                           tagName);
            }
            consumeTypeAttributes(parser, &type);
            consumeAlignasSpecifiers(parser, &type);
        } else if (hasEnumBody) {
            ASTNode** members = NULL;
            ASTNode** values = NULL;
            size_t count = 0;
            if (!parseInlineEnumBody(parser, &members, &values, &count)) {
                return parsedTypeDefault();
            }

            size_t trailingAttrCount = 0;
            ASTAttribute** trailingAttrs = parserParseAttributeSpecifiers(parser, &trailingAttrCount);

            if (!tagName) {
                char buffer[64];
                unsigned long long anonId = parser ? parser->anonTagCounter++ : 0ULL;
                snprintf(buffer, sizeof(buffer), "__anon_enum_%llu_%d", anonId, tagLine);
                tagName = strdup(buffer);
            }

            ASTNode* def = createEnumDefinitionNode(tagName ? tagName : "", members, values, count);
            if (def) {
                def->line = tagLine;
                if (def->enumDef.enumName) def->enumDef.enumName->line = tagLine;
                astNodeAppendAttributes(def, trailingAttrs, trailingAttrCount);
                trailingAttrs = NULL;
                trailingAttrCount = 0;
            } else {
                astAttributeListDestroy(trailingAttrs, trailingAttrCount);
                free(trailingAttrs);
            }

            type.tag = tag;
            type.kind = TYPE_ENUM;
            type.userTypeName = tagName ? strdup(tagName) : NULL;
            type.inlineEnumDef = def;
            sawBaseType = true;
            if (parser->ctx && tagName) {
                cc_add_tag(parser->ctx, CC_TAG_ENUM, tagName);
            }
            consumeTypeAttributes(parser, &type);
            consumeAlignasSpecifiers(parser, &type);
        } else {
            if (!tagName) {
                if (ctx == TYPECTX_Declaration) {
                    printParseError("Expected identifier after aggregate keyword", parser);
                }
                return parsedTypeDefault();
            }

            if (ctx == TYPECTX_Strict && !tagIsKnown(parser, tag, tagName)) {
                free(tagName);
                return parsedTypeDefault();
            }

            type.tag = tag;
            type.userTypeName = tagName;
            switch (tag) {
                case TAG_STRUCT: type.kind = TYPE_STRUCT; break;
                case TAG_UNION:  type.kind = TYPE_UNION;  break;
                case TAG_ENUM:   type.kind = TYPE_ENUM;   break;
                case TAG_NONE:
                default:
                    type.kind = TYPE_NAMED;
                    break;
            }
            sawBaseType = true;
        }
    } else if (!sawBaseType && parser->currentToken.type == TOKEN_IDENTIFIER) {
        const char* ident = parser->currentToken.value;
        bool hasTypeModifiers =
            type.isUnsigned || type.isSigned || type.isShort || type.isLong;
        bool hasQualifiers =
            type.isConst || type.isVolatile || type.isRestrict || type.isInline;
        bool known = (!hasTypeModifiers) && identifierMatchesKnownType(parser, ident);

        if (!known) {
            if (ctx == TYPECTX_Strict) {
                return parsedTypeDefault();
            }

            if (hasTypeModifiers || hasQualifiers) {
                // Treat combinations like "unsigned long" as integral types without
                // consuming the following identifier (likely the declarator name).
                type.kind = TYPE_PRIMITIVE;
                type.primitiveType = TOKEN_INT;
                sawBaseType = true;
            } else {
                type.kind = TYPE_NAMED;
                type.userTypeName = strdup(ident);
                advance(parser);
                sawBaseType = true;
            }
        } else {
            type.kind = TYPE_NAMED;
            type.userTypeName = strdup(ident);
            advance(parser);
            sawBaseType = true;
        }
    } else if (type.isUnsigned || type.isSigned || type.isShort || type.isLong || type.isComplex || type.isImaginary) {
        // Modifier with implicit int
        type.kind = TYPE_PRIMITIVE;
        type.primitiveType = TOKEN_INT;
        if (type.isComplex || type.isImaginary) {
            type.primitiveType = TOKEN_DOUBLE; // _Complex/_Imaginary without explicit base defaults to double
        }
        sawBaseType = true;
    } else {
        if (ctx == TYPECTX_Declaration) {
            printParseError("Expected type name", parser);
        }
        return type;
    }

    // Trailing qualifiers (e.g., "char const *p")
    for (;;) {
        switch (parser->currentToken.type) {
            case TOKEN_CONST:    type.isConst    = true; advance(parser); continue;
            case TOKEN_VOLATILE: type.isVolatile = true; advance(parser); continue;
            case TOKEN_RESTRICT: type.isRestrict = true; advance(parser); continue;
            case TOKEN_COMPLEX:  type.isComplex  = true; advance(parser); continue;
            case TOKEN_IMAGINARY: type.isImaginary = true; advance(parser); continue;
            default: break;
        }
        consumeTypeAttributes(parser, &type);
        break;
    }

    // Pointer qualifiers (only consumed eagerly outside declaration contexts)
    if (ctx != TYPECTX_Declaration) {
        while (parser->currentToken.type == TOKEN_ASTERISK ||
               (parser->ctx && cc_extensions_enabled(parser->ctx) &&
                parser->currentToken.type == TOKEN_BITWISE_XOR)) {
            advance(parser);
            PointerQuals quals = consumePointerQualifiers(parser);
            if (!parsedTypeAppendPointer(&type)) {
                return parsedTypeDefault();
            }
            if (type.derivationCount > 0) {
                TypeDerivation* slot = &type.derivations[type.derivationCount - 1];
                slot->as.pointer.isConst = quals.isConst;
                slot->as.pointer.isVolatile = quals.isVolatile;
                slot->as.pointer.isRestrict = quals.isRestrict;
            }
        }
    }

    if (!sawBaseType) {
        return parsedTypeDefault();
    }
    consumeTypeAttributes(parser, &type);
    consumeAlignasSpecifiers(parser, &type);

    return type;
}

ParsedType parseTypeCtx(Parser* parser, TypeContext ctx) {
    return parseTypeCore(parser, ctx);
}

ParsedType parseType(Parser* parser) {
    return parseTypeCore(parser, TYPECTX_Declaration);
}
