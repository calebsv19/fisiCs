#include "Parser/Helpers/parser_attributes.h"

#include "Parser/Helpers/parser_helpers.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char* buffer;
    size_t length;
} AttributeTextBuilder;

static void builderReset(AttributeTextBuilder* b) {
    if (!b) return;
    free(b->buffer);
    b->buffer = NULL;
    b->length = 0;
}

static bool builderAppendRaw(AttributeTextBuilder* b, const char* text) {
    if (!text) return true;
    size_t add = strlen(text);
    if (add == 0) return true;
    size_t newLen = b->length + add;
    char* next = (char*)realloc(b->buffer, newLen + 1);
    if (!next) {
        return false;
    }
    memcpy(next + b->length, text, add);
    next[newLen] = '\0';
    b->buffer = next;
    b->length = newLen;
    return true;
}

static bool needsSpace(const AttributeTextBuilder* b, const char* text) {
    if (!b || !text || b->length == 0 || text[0] == '\0') return false;
    char last = b->buffer[b->length - 1];
    char next = text[0];
    return (isalnum((unsigned char)last) || last == '_') &&
           (isalnum((unsigned char)next) || next == '_');
}

static bool builderAppend(AttributeTextBuilder* b, const char* text) {
    if (!text || text[0] == '\0') return true;
    if (needsSpace(b, text)) {
        if (!builderAppendRaw(b, " ")) return false;
    }
    return builderAppendRaw(b, text);
}

static char* builderDetach(AttributeTextBuilder* b) {
    if (!b) return NULL;
    char* out = b->buffer;
    b->buffer = NULL;
    b->length = 0;
    return out;
}

static bool expectToken(Parser* parser, TokenType type, const char* message) {
    if (parser->currentToken.type != type) {
        printParseError(message, parser);
        return false;
    }
    advance(parser);
    return true;
}

bool parserLookaheadStartsAttribute(Parser* parser) {
    if (!parser) return false;
    if (parser->currentToken.type == TOKEN_IDENTIFIER && parser->currentToken.value) {
        if (strcmp(parser->currentToken.value, "__attribute__") == 0) {
            return true;
        }
        if (strcmp(parser->currentToken.value, "__declspec") == 0) {
            return true;
        }
    }
    return parser->currentToken.type == TOKEN_LBRACKET &&
           parser->nextToken.type == TOKEN_LBRACKET;
}

static bool appendTokenText(AttributeTextBuilder* builder, const Token* tok) {
    if (!tok || !builder) return true;
    switch (tok->type) {
        case TOKEN_STRING: {
            if (!builderAppendRaw(builder, "\"")) return false;
            if (!builderAppendRaw(builder, tok->value ? tok->value : "")) return false;
            return builderAppendRaw(builder, "\"");
        }
        case TOKEN_CHAR_LITERAL: {
            if (!builderAppendRaw(builder, "'")) return false;
            if (!builderAppendRaw(builder, tok->value ? tok->value : "")) return false;
            return builderAppendRaw(builder, "'");
        }
        default:
            return builderAppend(builder, tok->value ? tok->value : "");
    }
}

static char* collectBalancedRegion(Parser* parser,
                                   TokenType openTok,
                                   TokenType closeTok,
                                   int startingDepth) {
    AttributeTextBuilder builder = {0};
    int depth = startingDepth;
    while (parser->currentToken.type != TOKEN_EOF) {
        Token tok = parser->currentToken;
        if (tok.type == closeTok) {
            depth--;
            if (depth <= 0) {
                advance(parser);
                return builderDetach(&builder);
            }
            if (!appendTokenText(&builder, &tok)) {
                builderReset(&builder);
                return NULL;
            }
            advance(parser);
            continue;
        }
        if (!appendTokenText(&builder, &tok)) {
            builderReset(&builder);
            return NULL;
        }
        advance(parser);
        if (tok.type == openTok) {
            depth++;
        }
    }
    builderReset(&builder);
    return NULL;
}

static ASTAttribute* parseGNUAttribute(Parser* parser) {
    advance(parser); /* consume __attribute__ */
    if (!expectToken(parser, TOKEN_LPAREN, "Expected '(' after __attribute__")) {
        return NULL;
    }
    if (!expectToken(parser, TOKEN_LPAREN, "Expected '(' to start attribute list")) {
        return NULL;
    }
    char* payload = collectBalancedRegion(parser, TOKEN_LPAREN, TOKEN_RPAREN, 1);
    if (!payload) {
        return NULL;
    }
    if (!expectToken(parser, TOKEN_RPAREN, "Expected ')' after attribute list")) {
        free(payload);
        return NULL;
    }
    ASTAttribute* attr = astAttributeCreate(AST_ATTRIBUTE_SYNTAX_GNU, payload);
    free(payload);
    return attr;
}

static ASTAttribute* parseCXXAttribute(Parser* parser) {
    advance(parser); /* consume '[' */
    advance(parser); /* consume second '[' */
    char* payload = collectBalancedRegion(parser, TOKEN_LBRACKET, TOKEN_RBRACKET, 1);
    if (!payload) {
        return NULL;
    }
    if (!expectToken(parser, TOKEN_RBRACKET, "Expected closing ']' for attribute")) {
        free(payload);
        return NULL;
    }
    ASTAttribute* attr = astAttributeCreate(AST_ATTRIBUTE_SYNTAX_CXX, payload);
    free(payload);
    return attr;
}

static ASTAttribute* parseDeclspecAttribute(Parser* parser) {
    advance(parser); /* consume __declspec */
    if (!expectToken(parser, TOKEN_LPAREN, "Expected '(' after __declspec")) {
        return NULL;
    }
    char* payload = collectBalancedRegion(parser, TOKEN_LPAREN, TOKEN_RPAREN, 1);
    if (!payload) {
        return NULL;
    }
    ASTAttribute* attr = astAttributeCreate(AST_ATTRIBUTE_SYNTAX_DECLSPEC, payload);
    free(payload);
    return attr;
}

static ASTAttribute* parseSingleAttribute(Parser* parser) {
    if (parser->currentToken.type == TOKEN_IDENTIFIER &&
        parser->currentToken.value &&
        strcmp(parser->currentToken.value, "__attribute__") == 0) {
        return parseGNUAttribute(parser);
    }
    if (parser->currentToken.type == TOKEN_IDENTIFIER &&
        parser->currentToken.value &&
        strcmp(parser->currentToken.value, "__declspec") == 0) {
        return parseDeclspecAttribute(parser);
    }
    if (parser->currentToken.type == TOKEN_LBRACKET &&
        parser->nextToken.type == TOKEN_LBRACKET) {
        return parseCXXAttribute(parser);
    }
    return NULL;
}

ASTAttribute** parserParseAttributeSpecifiers(Parser* parser, size_t* outCount) {
    if (outCount) {
        *outCount = 0;
    }
    if (!parserLookaheadStartsAttribute(parser)) {
        return NULL;
    }
    size_t capacity = 4;
    size_t count = 0;
    ASTAttribute** items = (ASTAttribute**)malloc(capacity * sizeof(ASTAttribute*));
    if (!items) {
        return NULL;
    }
    while (parserLookaheadStartsAttribute(parser)) {
        ASTAttribute* attr = parseSingleAttribute(parser);
        if (!attr) {
            break;
        }
        if (count >= capacity) {
            capacity *= 2;
            ASTAttribute** resized = (ASTAttribute**)realloc(items, capacity * sizeof(ASTAttribute*));
            if (!resized) {
                astAttributeListDestroy(items, count);
                free(items);
                return NULL;
            }
            items = resized;
        }
        items[count++] = attr;
    }
    if (count == 0) {
        free(items);
        return NULL;
    }
    if (outCount) {
        *outCount = count;
    }
    return items;
}
