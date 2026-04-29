// SPDX-License-Identifier: Apache-2.0

#include "parser_expr_pratt.h"

#include "Parser/Helpers/parser_helpers.h"
#include "parser_decl.h"
#include "parser_array.h"
#include "parser_func.h"
#include "Parser/Helpers/parser_lookahead.h"
#include "parser_expr.h"
#include "parser_main.h"
#include "AST/ast_node.h"
#include "Compiler/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool consumeBalancedParens(Parser* p) {
    if (p->currentToken.type != TOKEN_LPAREN) return false;
    int depth = 0;
    do {
        if (p->currentToken.type == TOKEN_LPAREN) depth++;
        else if (p->currentToken.type == TOKEN_RPAREN) depth--;
        advance(p);
        if (depth == 0) break;
        if (p->currentToken.type == TOKEN_EOF) return false;
    } while (1);
    return true;
}

void consumeAbstractDeclarator(Parser* p) {
    for (;;) {
        if (p->currentToken.type == TOKEN_ASTERISK ||
            (p->ctx && cc_compat_block_pointers_enabled(p->ctx) &&
             p->currentToken.type == TOKEN_BITWISE_XOR)) {
            advance(p);
            continue;
        }
        if (p->currentToken.type == TOKEN_LPAREN) {
            consumeBalancedParens(p);
            continue;
        }
        if (p->currentToken.type == TOKEN_LBRACKET) {
            advance(p);
            while (p->currentToken.type != TOKEN_RBRACKET &&
                   p->currentToken.type != TOKEN_EOF) {
                advance(p);
            }
            if (p->currentToken.type == TOKEN_RBRACKET) advance(p);
            continue;
        }
        break;
    }
}

void consumeAbstractDeclaratorIntoType(Parser* p, ParsedType* type) {
    if (!type) {
        consumeAbstractDeclarator(p);
        return;
    }

    while (p->currentToken.type == TOKEN_ASTERISK ||
           (p->ctx && cc_compat_block_pointers_enabled(p->ctx) &&
            p->currentToken.type == TOKEN_BITWISE_XOR)) {
        parsedTypeAppendPointer(type);
        advance(p);
    }

    bool sawComplex = false;
    while (p->currentToken.type == TOKEN_LPAREN) {
        sawComplex = true;
        consumeBalancedParens(p);
    }

    while (p->currentToken.type == TOKEN_LBRACKET) {
        advance(p);
        ASTNode* sizeExpr = NULL;
        if (p->currentToken.type != TOKEN_RBRACKET) {
            sizeExpr = parseExpressionPratt(p, 0);
        }
        if (p->currentToken.type != TOKEN_RBRACKET) {
            printParseError("Expected ']' after array declarator", p);
            return;
        }
        advance(p);

        parsedTypeAppendArray(type, sizeExpr, false);
        TypeDerivation* arr = parsedTypeGetMutableArrayDerivation(type, type->derivationCount - 1);
        if (arr && sizeExpr && sizeExpr->type == AST_NUMBER_LITERAL && sizeExpr->valueNode.value) {
            const char* literalText = sizeExpr->valueNode.value;
            arr->as.array.hasConstantSize = true;
            arr->as.array.constantSize = atoll(literalText);
        }
    }

    if (sawComplex) {
        consumeAbstractDeclarator(p);
    }
}

bool looksLikeParenTypeName(Parser* parser) {
    if (parser->currentToken.type != TOKEN_LPAREN) return false;

    Parser probe = cloneParserWithFreshLexer(parser);
    advance(&probe);

    ParsedType t = parseTypeCtx(&probe, TYPECTX_Strict);
    bool ok = (t.kind != TYPE_INVALID) && (probe.currentToken.type == TOKEN_RPAREN);
    freeParserClone(&probe);
    return ok;
}

ASTNode* parseCastExpressionPratt(Parser* parser, bool alreadyConsumedLParen) {
    PARSER_DEBUG_PRINTF("DEBUG: [Pratt] Entering parseCastExpressionPratt() with token '%s' (line %d)\n",
           parser->currentToken.value, parser->currentToken.line);

    if (!alreadyConsumedLParen) {
        if (parser->currentToken.type != TOKEN_LPAREN) {
            printParseError("Expected '(' to start cast", parser);
            return NULL;
        }
        advance(parser);
    }

    ParsedType castType = parseTypeCtx(parser, TYPECTX_Strict);
    if (castType.kind == TYPE_INVALID) {
        printParseError("Invalid type in cast expression", parser);
        return NULL;
    }

    ParsedDeclarator abstractDecl;
    if (parserParseDeclarator(parser, &castType, false, false, false, &abstractDecl)) {
        ParsedType clone = parsedTypeClone(&abstractDecl.type);
        parserDeclaratorDestroy(&abstractDecl);
        parsedTypeFree(&castType);
        castType = clone;
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' after cast type", parser);
        return NULL;
    }
    advance(parser);

    ASTNode* targetExpr = parseExpressionPratt(parser, 11);
    if (!targetExpr) {
        printParseError("Invalid expression after cast", parser);
        return NULL;
    }

    return createCastExpressionNode(castType, targetExpr);
}

ASTNode* parseCompoundLiteralPratt(Parser* parser, bool alreadyConsumedLParen) {
    if (!alreadyConsumedLParen) {
        if (parser->currentToken.type != TOKEN_LPAREN) {
            printParseError("Expected '(' to start compound literal type", parser);
            return NULL;
        }
        advance(parser);
    }

    ParsedType literalType = parseTypeCtx(parser, TYPECTX_Strict);
    if (literalType.kind == TYPE_INVALID) {
        printParseError("Invalid type in compound literal", parser);
        return NULL;
    }

    ParsedDeclarator abstractDecl;
    if (parserParseDeclarator(parser, &literalType, false, false, false, &abstractDecl)) {
        ParsedType clone = parsedTypeClone(&abstractDecl.type);
        parserDeclaratorDestroy(&abstractDecl);
        parsedTypeFree(&literalType);
        literalType = clone;
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' after compound literal type", parser);
        return NULL;
    }
    advance(parser);

    if (parser->currentToken.type != TOKEN_LBRACE) {
        printParseError("Expected '{' to start compound literal initializer", parser);
        return NULL;
    }
    advance(parser);

    DesignatedInit** items = NULL;
    size_t count = 0;

    while (parser->currentToken.type != TOKEN_RBRACE &&
           parser->currentToken.type != TOKEN_EOF) {
        DesignatedInit* di = NULL;

        if (parser->currentToken.type == TOKEN_DOT) {
            size_t fieldCount = 0;
            size_t fieldCap = 4;
            char** fieldNames = malloc(fieldCap * sizeof(char*));
            if (!fieldNames) return NULL;

            while (parser->currentToken.type == TOKEN_DOT) {
                advance(parser);
                if (parser->currentToken.type != TOKEN_IDENTIFIER) {
                    printParseError("Expected field name after '.' in designated initializer", parser);
                    free(fieldNames);
                    return NULL;
                }
                if (fieldCount >= fieldCap) {
                    fieldCap *= 2;
                    char** grown = realloc(fieldNames, fieldCap * sizeof(char*));
                    if (!grown) {
                        free(fieldNames);
                        return NULL;
                    }
                    fieldNames = grown;
                }
                fieldNames[fieldCount++] = strdup(parser->currentToken.value);
                advance(parser);
            }

            if (fieldCount == 0 || parser->currentToken.type != TOKEN_ASSIGN) {
                printParseError("Expected '=' after field designator", parser);
                for (size_t i = 0; i < fieldCount; ++i) free(fieldNames[i]);
                free(fieldNames);
                return NULL;
            }
            advance(parser);

            ASTNode* value = NULL;
            if (parser->currentToken.type == TOKEN_LBRACE) {
                size_t nestedCount = 0;
                DesignatedInit** nested = parseInitializerList(parser, literalType, &nestedCount);
                if (!nested) {
                    for (size_t i = 0; i < fieldCount; ++i) free(fieldNames[i]);
                    free(fieldNames);
                    return NULL;
                }
                value = createCompoundInit(nested, nestedCount);
            } else {
                value = parseExpressionPratt(parser, 0);
            }
            if (!value) {
                for (size_t i = 0; i < fieldCount; ++i) free(fieldNames[i]);
                free(fieldNames);
                return NULL;
            }

            if (fieldCount == 1) {
                di = createDesignatedInit(fieldNames[0], value);
            } else {
                ASTNode* nestedExpr = value;
                for (size_t i = fieldCount; i-- > 1;) {
                    DesignatedInit* fieldInit = createDesignatedInit(fieldNames[i], nestedExpr);
                    DesignatedInit** nestedEntries = malloc(sizeof(DesignatedInit*));
                    if (!fieldInit || !nestedEntries) {
                        for (size_t j = 0; j < fieldCount; ++j) free(fieldNames[j]);
                        free(fieldNames);
                        return NULL;
                    }
                    nestedEntries[0] = fieldInit;
                    nestedExpr = createCompoundInit(nestedEntries, 1);
                }
                di = createDesignatedInit(fieldNames[0], nestedExpr);
            }
            for (size_t i = 0; i < fieldCount; ++i) free(fieldNames[i]);
            free(fieldNames);
            if (!di) return NULL;
        } else if (parser->currentToken.type == TOKEN_LBRACKET) {
            advance(parser);

            ASTNode* idx = parseExpressionPratt(parser, 0);
            if (!idx) return NULL;

            if (parser->currentToken.type != TOKEN_RBRACKET) {
                printParseError("Expected ']' after index designator", parser);
                return NULL;
            }
            advance(parser);

            if (parser->currentToken.type != TOKEN_ASSIGN) {
                printParseError("Expected '=' after index designator", parser);
                return NULL;
            }
            advance(parser);

            ASTNode* value = NULL;
            if (parser->currentToken.type == TOKEN_LBRACE) {
                size_t nestedCount = 0;
                DesignatedInit** nested = parseInitializerList(parser, literalType, &nestedCount);
                if (!nested) return NULL;
                value = createCompoundInit(nested, nestedCount);
            } else {
                value = parseExpressionPratt(parser, 0);
            }
            if (!value) return NULL;

            di = createSimpleInit(value);
            if (!di) return NULL;
            di->indexExpr = idx;
        } else {
            ASTNode* value = NULL;
            if (parser->currentToken.type == TOKEN_LBRACE) {
                size_t nestedCount = 0;
                DesignatedInit** nested = parseInitializerList(parser, literalType, &nestedCount);
                if (!nested) return NULL;
                value = createCompoundInit(nested, nestedCount);
            } else {
                value = parseExpressionPratt(parser, 0);
            }
            if (!value) return NULL;

            di = createSimpleInit(value);
            if (!di) return NULL;
        }

        DesignatedInit** newItems = realloc(items, (count + 1) * sizeof(*newItems));
        if (!newItems) {
            if (parser && parser->ctx) {
                compiler_report_diag(parser->ctx,
                                     parser->currentToken.location,
                                     DIAG_ERROR,
                                     CDIAG_PARSER_GENERIC,
                                     NULL,
                                     "out of memory while parsing compound literal");
            }
            return NULL;
        }
        items = newItems;
        items[count++] = di;

        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser);
            if (parser->currentToken.type == TOKEN_RBRACE) break;
        }
    }

    if (parser->currentToken.type != TOKEN_RBRACE) {
        printParseError("Expected '}' to close compound literal initializer", parser);
        return NULL;
    }
    advance(parser);

    return createCompoundLiteralNode(literalType, items, count);
}

ASTNode* parseSizeofExpressionPratt(Parser* parser, const Token* sizeofToken) {
    PARSER_DEBUG_PRINTF("DEBUG [Pratt]: entering parseSizeofExpressionPratt(), next token = '%s' (type %d)\n",
           parser->currentToken.value, parser->currentToken.type);

    if (parser->currentToken.type == TOKEN_LPAREN) {
        Parser temp = cloneParserWithFreshLexer(parser);
        advance(&temp);

        ParsedType probeType = parseTypeCtx(&temp, TYPECTX_Strict);
        bool looksLikeType = (probeType.kind != TYPE_INVALID);
        if (looksLikeType) {
            ParsedDeclarator probeDecl;
            if (parserParseDeclarator(&temp, &probeType, false, false, false, &probeDecl)) {
                parserDeclaratorDestroy(&probeDecl);
                looksLikeType = (temp.currentToken.type == TOKEN_RPAREN);
            } else {
                looksLikeType = false;
            }
        }
        parsedTypeFree(&probeType);
        freeParserClone(&temp);

        if (looksLikeType) {
            advance(parser);
            ParsedType realType = parseTypeCtx(parser, TYPECTX_Strict);
            if (realType.kind == TYPE_INVALID) {
                printParseError("Invalid type in sizeof(type)", parser);
                return NULL;
            }
            ParsedDeclarator abstractDecl;
            if (!parserParseDeclarator(parser, &realType, false, false, false, &abstractDecl)) {
                parsedTypeFree(&realType);
                printParseError("Invalid abstract declarator in sizeof(type)", parser);
                return NULL;
            }
            parsedTypeFree(&realType);
            realType = abstractDecl.type;
            memset(&abstractDecl.type, 0, sizeof(abstractDecl.type));
            parserDeclaratorDestroy(&abstractDecl);
            if (parser->currentToken.type != TOKEN_RPAREN) {
                printParseError("Expected ')' after sizeof(type)", parser);
                return NULL;
            }
            advance(parser);
            ASTNode* node = createSizeofNode(createParsedTypeNode(realType));
            if (node && sizeofToken) {
                astNodeSetProvenance(node, sizeofToken);
            }
            return node;
        }
    }

    ASTNode* operand = parseExpressionPratt(parser, 11);
    if (!operand) {
        printParseError("Invalid operand for sizeof", parser);
        return NULL;
    }
    ASTNode* node = createSizeofNode(operand);
    if (node && sizeofToken) {
        astNodeSetProvenance(node, sizeofToken);
    }
    return node;
}

ASTNode* parseAlignofExpressionPratt(Parser* parser, const Token* alignofToken) {
    if (parser->currentToken.type == TOKEN_LPAREN) {
        Parser temp = cloneParserWithFreshLexer(parser);
        advance(&temp);
        ParsedType probeType = parseTypeCtx(&temp, TYPECTX_Strict);
        bool looksLikeType = (probeType.kind != TYPE_INVALID);
        if (looksLikeType) {
            ParsedDeclarator probeDecl;
            if (parserParseDeclarator(&temp, &probeType, false, false, false, &probeDecl)) {
                parserDeclaratorDestroy(&probeDecl);
                looksLikeType = (temp.currentToken.type == TOKEN_RPAREN);
            } else {
                looksLikeType = false;
            }
        }
        parsedTypeFree(&probeType);
        freeParserClone(&temp);

        if (looksLikeType) {
            advance(parser);
            ParsedType realType = parseTypeCtx(parser, TYPECTX_Strict);
            if (realType.kind == TYPE_INVALID) {
                printParseError("Invalid type in alignof(type)", parser);
                return NULL;
            }
            ParsedDeclarator abstractDecl;
            if (!parserParseDeclarator(parser, &realType, false, false, false, &abstractDecl)) {
                parsedTypeFree(&realType);
                printParseError("Invalid abstract declarator in alignof(type)", parser);
                return NULL;
            }
            parsedTypeFree(&realType);
            realType = abstractDecl.type;
            memset(&abstractDecl.type, 0, sizeof(abstractDecl.type));
            parserDeclaratorDestroy(&abstractDecl);
            if (parser->currentToken.type != TOKEN_RPAREN) {
                printParseError("Expected ')' after alignof(type)", parser);
                return NULL;
            }
            advance(parser);
            ASTNode* node = createAlignofNode(createParsedTypeNode(realType));
            if (node && alignofToken) {
                astNodeSetProvenance(node, alignofToken);
            }
            return node;
        }
    }

    ASTNode* operand = parseExpressionPratt(parser, 11);
    if (!operand) {
        printParseError("Invalid operand for alignof", parser);
        return NULL;
    }
    ASTNode* node = createAlignofNode(operand);
    if (node && alignofToken) {
        astNodeSetProvenance(node, alignofToken);
    }
    return node;
}
