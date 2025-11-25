#include "Parser/Helpers/parsed_type.h"
#include "Parser/parser.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Compiler/compiler_context.h"

#include <stdlib.h>
#include <string.h>

// Convert TokenType to string for debugging and printing
const char* getTokenTypeName(TokenType type) {
    switch (type) {
        case TOKEN_INT: return "int";
        case TOKEN_FLOAT: return "float";
        case TOKEN_DOUBLE: return "double";
        case TOKEN_CHAR: return "char";
        case TOKEN_BOOL: return "bool";
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
}

static ParsedType parsedTypeDefault(void) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_INVALID;
    t.tag = TAG_NONE;
    t.primitiveType = TOKEN_VOID;
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
    return copy;
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

static void consumePointerQualifiers(Parser* parser) {
    while (parser->currentToken.type == TOKEN_CONST ||
           parser->currentToken.type == TOKEN_VOLATILE ||
           parser->currentToken.type == TOKEN_RESTRICT) {
        advance(parser);
    }
}

static ParsedType parseTypeCore(Parser* parser, TypeContext ctx) {
    ParsedType type = parsedTypeDefault();
    if (!parser) {
        return type;
    }

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
            default: break;
        }
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
        advance(parser); // consume keyword

        if (parser->currentToken.type != TOKEN_IDENTIFIER) {
            if (ctx == TYPECTX_Declaration) {
                printParseError("Expected identifier after aggregate keyword", parser);
            }
            return parsedTypeDefault();
        }

        const char* tagName = parser->currentToken.value;
        TagKind tag = tagKindFromToken(tagToken);
        if (ctx == TYPECTX_Strict && !tagIsKnown(parser, tag, tagName)) {
            return parsedTypeDefault();
        }

        type.tag = tag;
        type.userTypeName = strdup(tagName);
        switch (tag) {
            case TAG_STRUCT: type.kind = TYPE_STRUCT; break;
            case TAG_UNION:  type.kind = TYPE_UNION;  break;
            case TAG_ENUM:   type.kind = TYPE_ENUM;   break;
            case TAG_NONE:
            default:
                type.kind = TYPE_NAMED;
                break;
        }
        advance(parser);
        sawBaseType = true;
    } else if (!sawBaseType && parser->currentToken.type == TOKEN_IDENTIFIER) {
        const char* ident = parser->currentToken.value;
        bool known = identifierMatchesKnownType(parser, ident);
        bool hasQualifierSpecs =
            type.isUnsigned || type.isSigned || type.isShort || type.isLong ||
            type.isConst || type.isVolatile || type.isRestrict || type.isInline;

        if (!known) {
            if (ctx == TYPECTX_Strict) {
                return parsedTypeDefault();
            }

            if (hasQualifierSpecs) {
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
    } else if (type.isUnsigned || type.isSigned || type.isShort || type.isLong) {
        // Modifier with implicit int
        type.kind = TYPE_PRIMITIVE;
        type.primitiveType = TOKEN_INT;
        sawBaseType = true;
    } else {
        if (ctx == TYPECTX_Declaration) {
            printParseError("Expected type name", parser);
        }
        return type;
    }

    // Pointer qualifiers (only consumed eagerly outside declaration contexts)
    if (ctx != TYPECTX_Declaration) {
        while (parser->currentToken.type == TOKEN_ASTERISK) {
            type.pointerDepth += 1;
            advance(parser);
            consumePointerQualifiers(parser);
        }
    }

    if (!sawBaseType) {
        return parsedTypeDefault();
    }

    return type;
}

ParsedType parseTypeCtx(Parser* parser, TypeContext ctx) {
    return parseTypeCore(parser, ctx);
}

ParsedType parseType(Parser* parser) {
    return parseTypeCore(parser, TYPECTX_Declaration);
}
