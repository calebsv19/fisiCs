// SPDX-License-Identifier: Apache-2.0

#include "parser_decl.h"

#include "Parser/Helpers/parser_attributes.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Parser/Helpers/parser_lookahead.h"
#include "parser_array.h"
#include "parser_func.h"

#include "Compiler/compiler_context.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FunctionTypeParseResult {
    ParsedType* params;
    size_t count;
    bool isVariadic;
} FunctionTypeParseResult;

static bool looksLikeIdentifierParamList(Parser* parser);
static bool parseDeclaratorInternal(Parser* parser,
                                    ParsedType* type,
                                    ParsedDeclarator* decl,
                                    bool trackDirectFunction,
                                    bool requireIdentifier,
                                    bool captureFunctionParams,
                                    bool topLevel);
static bool parseFunctionTypeParameterList(Parser* parser, FunctionTypeParseResult* out);
static void freeFunctionTypeParseResult(FunctionTypeParseResult* out);
static bool parser_allows_block_pointers(Parser* parser);
static bool parser_allows_atomic_keyword(Parser* parser);
static void consumePointerQualifiers(Parser* parser, PointerDeclaratorLayer* layer);
static void normalizeGroupedArrayPointer(ParsedType* type);
static void parsedDeclaratorInit(ParsedDeclarator* decl);
static ParsedType* buildParamTypesFromDecls(ASTNode** params, size_t paramCount);
static bool parseFunctionSuffix(Parser* parser,
                                ParsedType* type,
                                ParsedDeclarator* decl,
                                bool trackDirectFunction,
                                bool captureFunctionParams,
                                bool topLevel,
                                bool groupedDeclarator);

// Probe-only helper: determines whether the token stream up to the next ')'
// consists solely of identifiers separated by commas (K&R-style header).
static bool looksLikeIdentifierParamList(Parser* parser) {
    Parser probe = cloneParserWithFreshLexer(parser);
    bool sawIdent = false;
    while (probe.currentToken.type != TOKEN_RPAREN &&
           probe.currentToken.type != TOKEN_EOF) {
        if (probe.currentToken.type != TOKEN_IDENTIFIER) {
            freeParserClone(&probe);
            return false;
        }
        sawIdent = true;
        advance(&probe);
        if (probe.currentToken.type == TOKEN_COMMA) {
            advance(&probe);
            continue;
        }
        if (probe.currentToken.type != TOKEN_RPAREN) {
            freeParserClone(&probe);
            return false;
        }
    }
    bool ok = sawIdent && probe.currentToken.type == TOKEN_RPAREN;
    if (ok) {
        Token after = peekNextToken(&probe);
        if (after.type == TOKEN_SEMICOLON) {
            ok = false;
        }
    }
    freeParserClone(&probe);
    return ok;
}

static bool parser_allows_block_pointers(Parser* parser) {
    return parser && parser->ctx && cc_extensions_enabled(parser->ctx);
}

static bool parser_allows_atomic_keyword(Parser* parser) {
    if (!parser || !parser->ctx) return false;
    if (cc_extensions_enabled(parser->ctx)) return true;
    CCDialect dialect = cc_get_language_dialect(parser->ctx);
    return dialect == CC_DIALECT_C11 || dialect == CC_DIALECT_C17;
}

static void consumePointerQualifiers(Parser* parser, PointerDeclaratorLayer* layer) {
    while (parser->currentToken.type == TOKEN_CONST ||
           parser->currentToken.type == TOKEN_VOLATILE ||
           parser->currentToken.type == TOKEN_RESTRICT ||
           parser->currentToken.type == TOKEN_ATOMIC) {
        if (parser->currentToken.type == TOKEN_ATOMIC) {
            if (!parser_allows_atomic_keyword(parser)) {
                printParseError("_Atomic is not supported yet", parser);
                markParserFatalError(parser);
            }
            advance(parser);
            continue;
        }
        if (layer) {
            if (parser->currentToken.type == TOKEN_CONST) {
                layer->isConst = true;
            } else if (parser->currentToken.type == TOKEN_VOLATILE) {
                layer->isVolatile = true;
            } else if (parser->currentToken.type == TOKEN_RESTRICT) {
                layer->isRestrict = true;
            }
        }
        advance(parser);
    }
}

PointerChain parsePointerChain(Parser* parser) {
    PointerChain chain = {0};
    while (parser->currentToken.type == TOKEN_ASTERISK ||
           (parser_allows_block_pointers(parser) && parser->currentToken.type == TOKEN_BITWISE_XOR)) {
        advance(parser);
        PointerDeclaratorLayer layer = {0};
        consumePointerQualifiers(parser, &layer);
        PointerDeclaratorLayer* grown = realloc(chain.layers, (chain.count + 1) * sizeof(PointerDeclaratorLayer));
        if (!grown) {
            pointerChainFree(&chain);
            chain.layers = NULL;
            chain.count = 0;
            return chain;
        }
        chain.layers = grown;
        chain.layers[chain.count++] = layer;
    }
    return chain;
}

void pointerChainFree(PointerChain* chain) {
    if (!chain) return;
    free(chain->layers);
    chain->layers = NULL;
    chain->count = 0;
}

static void normalizeGroupedArrayPointer(ParsedType* type) {
    if (!type || type->derivationCount < 2 || !type->derivations) {
        return;
    }

    size_t arrayCount = 0;
    size_t firstArray = (size_t)-1;
    for (size_t i = 0; i < type->derivationCount; ++i) {
        if (type->derivations[i].kind == TYPE_DERIVATION_ARRAY) {
            arrayCount++;
            firstArray = i;
        }
    }
    if (arrayCount != 1) {
        return;
    }
    if (firstArray == (size_t)-1) {
        return;
    }

    bool hasPointerAfter = false;
    for (size_t i = firstArray + 1; i < type->derivationCount; ++i) {
        if (type->derivations[i].kind == TYPE_DERIVATION_POINTER) {
            hasPointerAfter = true;
            break;
        }
    }
    if (!hasPointerAfter) {
        return;
    }

    TypeDerivation* reordered = (TypeDerivation*)malloc(type->derivationCount * sizeof(TypeDerivation));
    if (!reordered) {
        return;
    }

    size_t out = 0;
    for (size_t i = firstArray + 1; i < type->derivationCount; ++i) {
        if (type->derivations[i].kind == TYPE_DERIVATION_POINTER) {
            reordered[out++] = type->derivations[i];
        }
    }
    for (size_t i = 0; i < type->derivationCount; ++i) {
        bool moved = (i > firstArray && type->derivations[i].kind == TYPE_DERIVATION_POINTER);
        if (moved) {
            continue;
        }
        reordered[out++] = type->derivations[i];
    }

    memcpy(type->derivations, reordered, type->derivationCount * sizeof(TypeDerivation));
    free(reordered);
}

void applyPointerChainToType(ParsedType* type, const PointerChain* chain) {
    if (!type || !chain || chain->count == 0) {
        return;
    }
    /*
     * Pointer declarators are parsed left-to-right ("* const *"), but the
     * derived type should apply the pointer closest to the identifier first.
     * Reverse application preserves qualifier ownership for nested pointers.
     */
    for (size_t i = chain->count; i > 0; --i) {
        if (!parsedTypeAppendPointer(type)) {
            continue;
        }
        if (type->derivationCount == 0) continue;
        TypeDerivation* slot = &type->derivations[type->derivationCount - 1];
        const PointerDeclaratorLayer* layer = &chain->layers[i - 1];
        slot->as.pointer.isConst = layer->isConst;
        slot->as.pointer.isVolatile = layer->isVolatile;
        slot->as.pointer.isRestrict = layer->isRestrict;
    }
}

bool parserConsumeArraySuffixes(Parser* parser, ParsedType* type) {
    if (!parser || !type) return false;
    while (parser->currentToken.type == TOKEN_LBRACKET) {
        bool isVLA = false;
        ParsedArrayInfo arrayInfo = {0};
        ASTNode* sizeExpr = parseArraySize(parser, &isVLA, &arrayInfo);
        if (!parsedTypeAppendArray(type, sizeExpr, isVLA)) {
            return false;
        }
        TypeDerivation* arr = parsedTypeGetMutableArrayDerivation(type, type->derivationCount - 1);
        if (arr) {
            arr->as.array.hasStatic = arrayInfo.hasStatic;
            arr->as.array.qualConst = arrayInfo.qualConst;
            arr->as.array.qualVolatile = arrayInfo.qualVolatile;
            arr->as.array.qualRestrict = arrayInfo.qualRestrict;
        }
    }
    return true;
}

static void parsedDeclaratorInit(ParsedDeclarator* decl) {
    if (!decl) return;
    memset(decl, 0, sizeof(*decl));
    decl->type.kind = TYPE_INVALID;
    decl->type.tag = TAG_NONE;
    decl->identifier = NULL;
    decl->functionParameters = NULL;
    decl->functionParamCount = 0;
    decl->functionIsVariadic = false;
    decl->declaresFunction = false;
}

void parserDeclaratorDestroy(ParsedDeclarator* decl) {
    if (!decl) return;
    parsedTypeFree(&decl->type);
    decl->identifier = NULL;
    decl->functionParameters = NULL;
    decl->functionParamCount = 0;
    decl->functionIsVariadic = false;
    decl->declaresFunction = false;
}

static bool parseFunctionTypeParameterList(Parser* parser, FunctionTypeParseResult* out) {
    if (!out) return false;
    out->params = NULL;
    out->count = 0;
    out->isVariadic = false;

    size_t capacity = 0;
    bool onlyVoid = false;
    if (parser->currentToken.type == TOKEN_RPAREN) {
        return true;
    }

    // K&R-style identifier-only parameter list: (a, b, c)
    if (looksLikeIdentifierParamList(parser)) {
        // Consume the identifiers and commas, leave count/params empty to
        // represent an old-style, non-prototype function type.
        while (parser->currentToken.type != TOKEN_RPAREN &&
               parser->currentToken.type != TOKEN_EOF) {
            if (parser->currentToken.type == TOKEN_IDENTIFIER) {
                advance(parser);
                if (parser->currentToken.type == TOKEN_COMMA) {
                    advance(parser);
                } else if (parser->currentToken.type != TOKEN_RPAREN) {
                    return false;
                }
            } else {
                return false;
            }
        }
        return parser->currentToken.type == TOKEN_RPAREN;
    }

    while (parser->currentToken.type != TOKEN_RPAREN &&
           parser->currentToken.type != TOKEN_EOF) {
        if (parser->currentToken.type == TOKEN_ELLIPSIS) {
            out->isVariadic = true;
            advance(parser);
            break;
        }

        if (parser->currentToken.type == TOKEN_VOID) {
            Token next = peekNextToken(parser);
            if (next.type == TOKEN_RPAREN && out->count == 0) {
                advance(parser);
                onlyVoid = true;
                break;
            }
        }

        size_t leadingAttrCount = 0;
        ASTAttribute** leadingAttrs = parserParseAttributeSpecifiers(parser, &leadingAttrCount);
        ParsedType baseType = parseType(parser);
        if (baseType.kind == TYPE_INVALID) {
            parsedTypeAdoptAttributes(NULL, leadingAttrs, leadingAttrCount);
            return false;
        }

        ParsedDeclarator paramDecl;
        if (!parserParseDeclarator(parser, &baseType, false, false, false, &paramDecl)) {
            parsedTypeFree(&baseType);
            parsedTypeAdoptAttributes(NULL, leadingAttrs, leadingAttrCount);
            return false;
        }
        ParsedType paramType = parsedTypeClone(&paramDecl.type);
        parserDeclaratorDestroy(&paramDecl);
        parsedTypeFree(&baseType);

        parsedTypeAdoptAttributes(&paramType, leadingAttrs, leadingAttrCount);
        parsedTypeAdjustArrayParameter(&paramType);

        if (out->count == capacity) {
            size_t newCap = capacity == 0 ? 4 : capacity * 2;
            ParsedType* grown = realloc(out->params, newCap * sizeof(ParsedType));
            if (!grown) {
                parsedTypeFree(&paramType);
                return false;
            }
            out->params = grown;
            capacity = newCap;
        }
        out->params[out->count++] = paramType;

        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser);
            continue;
        }
        break;
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        return false;
    }

    if (onlyVoid) {
        free(out->params);
        out->params = NULL;
        out->count = 0;
        out->isVariadic = false;
    }
    return true;
}

static void freeFunctionTypeParseResult(FunctionTypeParseResult* out) {
    if (!out) return;
    free(out->params);
    out->params = NULL;
    out->count = 0;
    out->isVariadic = false;
}

static ParsedType* buildParamTypesFromDecls(ASTNode** params, size_t paramCount) {
    if (!params || paramCount == 0) {
        return NULL;
    }
    ParsedType* types = malloc(paramCount * sizeof(ParsedType));
    if (!types) {
        return NULL;
    }
    for (size_t i = 0; i < paramCount; ++i) {
        const ParsedType* paramType = astVarDeclTypeAt(params[i], 0);
        if (!paramType) {
            memset(&types[i], 0, sizeof(ParsedType));
            types[i].kind = TYPE_INVALID;
            types[i].tag = TAG_NONE;
        } else {
            types[i] = parsedTypeClone(paramType);
        }
    }
    return types;
}

static bool parseFunctionSuffix(Parser* parser,
                                ParsedType* type,
                                ParsedDeclarator* decl,
                                bool trackDirectFunction,
                                bool captureFunctionParams,
                                bool topLevel,
                                bool groupedDeclarator) {
    advance(parser); // consume '('
    FunctionTypeParseResult info = {0};
    ASTNode** paramList = NULL;
    size_t paramCount = 0;
    bool isVariadic = false;
    ParsedType* paramTypes = NULL;
    bool captureDeclParams = captureFunctionParams &&
                             topLevel &&
                             !groupedDeclarator &&
                             decl != NULL &&
                             decl->functionParameters == NULL;

    if (captureDeclParams) {
        paramList = parseParameterList(parser, &paramCount, &isVariadic);
        if (!paramList && paramCount == 0) {
            printParseError("Invalid function parameter list", parser);
            return false;
        }
        if (parser->currentToken.type != TOKEN_RPAREN) {
            printParseError("Expected ')' after parameter list", parser);
            return false;
        }
        paramTypes = buildParamTypesFromDecls(paramList, paramCount);
        if (paramCount > 0 && !paramTypes) {
            printParseError("Out of memory building function parameter types", parser);
            return false;
        }
        info.params = paramTypes;
        info.count = paramCount;
        info.isVariadic = isVariadic;
        decl->functionParameters = paramList;
        decl->functionParamCount = paramCount;
        decl->functionIsVariadic = isVariadic;
    } else {
        if (!parseFunctionTypeParameterList(parser, &info)) {
            freeFunctionTypeParseResult(&info);
            printParseError("Invalid function parameter list", parser);
            return false;
        }
        if (parser->currentToken.type != TOKEN_RPAREN) {
            freeFunctionTypeParseResult(&info);
            printParseError("Expected ')' after parameter list", parser);
            return false;
        }
    }

    advance(parser); // consume ')'

    bool appended = parsedTypeAppendFunction(type,
                                            info.params,
                                            info.count,
                                            info.isVariadic);
    if (groupedDeclarator) {
        bool hasGroupedPointer = false;
        for (size_t i = 0; i < type->derivationCount; ++i) {
            if (type->derivations[i].kind == TYPE_DERIVATION_POINTER) {
                hasGroupedPointer = true;
                break;
            }
        }
        if (hasGroupedPointer) {
            parsedTypeSetFunctionPointer(type, info.count, info.params);
        }
    }
    if (captureFunctionParams) {
        free(paramTypes);
    } else {
        freeFunctionTypeParseResult(&info);
    }
    if (!appended) {
        printParseError("Failed to record function type", parser);
        return false;
    }
    if (trackDirectFunction && topLevel && !groupedDeclarator) {
        decl->declaresFunction = true;
    }
    return true;
}

static bool parseDeclaratorInternal(Parser* parser,
                                    ParsedType* type,
                                    ParsedDeclarator* decl,
                                    bool trackDirectFunction,
                                    bool requireIdentifier,
                                    bool captureFunctionParams,
                                    bool topLevel) {
    size_t leadingAttrCount = 0;
    ASTAttribute** leadingAttrs = parserParseAttributeSpecifiers(parser, &leadingAttrCount);
    parsedTypeAdoptAttributes(type, leadingAttrs, leadingAttrCount);

    PointerChain chain = {0};
    PointerChain leading = parsePointerChain(parser);
    if (leading.count > 0) {
        PointerDeclaratorLayer* grown =
            realloc(chain.layers, (chain.count + leading.count) * sizeof(PointerDeclaratorLayer));
        if (grown) {
            chain.layers = grown;
            memcpy(chain.layers + chain.count, leading.layers, leading.count * sizeof(PointerDeclaratorLayer));
            chain.count += leading.count;
        }
    }
    pointerChainFree(&leading);

    for (;;) {
        size_t midAttrCount = 0;
        ASTAttribute** midAttrs = parserParseAttributeSpecifiers(parser, &midAttrCount);
        parsedTypeAdoptAttributes(type, midAttrs, midAttrCount);

        if (parser->currentToken.type != TOKEN_ASTERISK) {
            break;
        }
        PointerChain extra = parsePointerChain(parser);
        if (extra.count > 0) {
            PointerDeclaratorLayer* grown =
                realloc(chain.layers, (chain.count + extra.count) * sizeof(PointerDeclaratorLayer));
            if (grown) {
                chain.layers = grown;
                memcpy(chain.layers + chain.count, extra.layers, extra.count * sizeof(PointerDeclaratorLayer));
                chain.count += extra.count;
            }
        }
        pointerChainFree(&extra);
    }

    bool sawIdentifier = decl->identifier != NULL;
    bool groupedDeclarator = false;

    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        if (!decl->identifier) {
            decl->identifier = createIdentifierNode(parser->currentToken.value);
            astNodeSetProvenance(decl->identifier, &parser->currentToken);
        }
        advance(parser);
        sawIdentifier = true;
    } else if (parser->currentToken.type == TOKEN_LPAREN) {
        advance(parser);
        if (!parseDeclaratorInternal(parser,
                                     type,
                                     decl,
                                     trackDirectFunction,
                                     requireIdentifier,
                                     captureFunctionParams,
                                     topLevel)) {
            return false;
        }
        if (parser->currentToken.type != TOKEN_RPAREN) {
            printParseError("Expected ')' in declarator", parser);
            return false;
        }
        advance(parser);
        groupedDeclarator = true;
    } else {
        if (requireIdentifier && !sawIdentifier) {
            printParseError("Expected identifier in declarator", parser);
            return false;
        }
    }

    bool sawArraySuffix = false;
    while (parser->currentToken.type == TOKEN_LBRACKET ||
           parser->currentToken.type == TOKEN_LPAREN) {
        if (parser->currentToken.type == TOKEN_LBRACKET) {
            bool isVLA = false;
            ParsedArrayInfo arrayInfo = {0};
            ASTNode* sizeExpr = parseArraySize(parser, &isVLA, &arrayInfo);
            if (!parsedTypeAppendArray(type, sizeExpr, isVLA)) {
                printParseError("Failed to parse array suffix", parser);
                return false;
            }
            TypeDerivation* arr = parsedTypeGetMutableArrayDerivation(type, type->derivationCount - 1);
            if (arr) {
                arr->as.array.hasStatic = arrayInfo.hasStatic;
                arr->as.array.qualConst = arrayInfo.qualConst;
                arr->as.array.qualVolatile = arrayInfo.qualVolatile;
                arr->as.array.qualRestrict = arrayInfo.qualRestrict;
            }
            if (arr && sizeExpr && sizeExpr->type == AST_NUMBER_LITERAL && sizeExpr->valueNode.value) {
                arr->as.array.hasConstantSize = true;
                arr->as.array.constantSize = atoll(sizeExpr->valueNode.value);
            }
            sawArraySuffix = true;
        } else {
            if (!parseFunctionSuffix(parser,
                                     type,
                                     decl,
                                     trackDirectFunction,
                                     captureFunctionParams,
                                     topLevel,
                                     groupedDeclarator)) {
                return false;
            }
        }
    }

    size_t suffixAttrCount = 0;
    ASTAttribute** suffixAttrs = parserParseAttributeSpecifiers(parser, &suffixAttrCount);
    parsedTypeAdoptAttributes(type, suffixAttrs, suffixAttrCount);

    applyPointerChainToType(type, &chain);
    pointerChainFree(&chain);
    if (groupedDeclarator && sawArraySuffix) {
        normalizeGroupedArrayPointer(type);
    }
    parsedTypeNormalizeFunctionPointer(type);
    if (getenv("DEBUG_DECLARATOR") && decl && decl->identifier && decl->identifier->valueNode.value) {
        fprintf(stderr, "[decl] %s: pointerDepth=%d derivations=", decl->identifier->valueNode.value, type->pointerDepth);
        for (size_t i = 0; i < type->derivationCount; ++i) {
            const TypeDerivation* deriv = &type->derivations[i];
            const char* kind = "?";
            if (deriv->kind == TYPE_DERIVATION_ARRAY) kind = "array";
            else if (deriv->kind == TYPE_DERIVATION_POINTER) kind = "ptr";
            else if (deriv->kind == TYPE_DERIVATION_FUNCTION) kind = "func";
            fprintf(stderr, "%s%s", (i ? "," : ""), kind);
        }
        fprintf(stderr, "\n");
    }

    if (requireIdentifier && !decl->identifier) {
        printParseError("Expected identifier in declarator", parser);
        return false;
    }
    return true;
}

bool parserParseDeclarator(Parser* parser,
                           const ParsedType* baseType,
                           bool trackDirectFunction,
                           bool requireIdentifier,
                           bool captureFunctionParams,
                           ParsedDeclarator* out) {
    if (!parser || !baseType || !out) {
        return false;
    }
    parsedDeclaratorInit(out);
    out->type = parsedTypeClone(baseType);
    if (out->type.kind == TYPE_INVALID && baseType->kind != TYPE_INVALID) {
        printParseError("Failed to clone base type for declarator", parser);
        return false;
    }
    if (!parseDeclaratorInternal(parser,
                                 &out->type,
                                 out,
                                 trackDirectFunction,
                                 requireIdentifier,
                                 captureFunctionParams,
                                 true)) {
        parserDeclaratorDestroy(out);
        return false;
    }
    return true;
}
