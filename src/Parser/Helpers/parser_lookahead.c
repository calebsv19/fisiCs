#include "Parser/Helpers/parser_lookahead.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Parser/Expr/parser_expr_pratt.h"
#include "parser_decl.h"

void printParserState(const char* label, Parser* parser) {
    printf("\n=== PARSER STATE: %s ===\n", label);
    printf("currentToken:      %-10s at line %d\n", parser->currentToken.value, parser->currentToken.line);
    printf("nextToken:         %-10s at line %d\n", parser->nextToken.value, parser->nextToken.line);
    printf("nextNextToken:     %-10s at line %d\n", parser->nextNextToken.value, parser->nextNextToken.line);
    printf("nextNextNextToken: %-10s at line %d\n", parser->nextNextNextToken.value, parser->nextNextNextToken.line);
    if (parser->tokenBuffer) {
        printf("Token buffer size: %zu\n", parser->tokenBuffer->count);
        printf("Cursor index:      %zu\n", parser->cursor);
    }
    printf("===========================\n\n");
}


bool looksLikeTypeDeclaration(Parser* parser) {
    if (parser->currentToken.type == TOKEN_TYPEDEF) {
        return false; // handled explicitly via parseTypedef
    }

    // Fast path: obvious type-start tokens
    switch (parser->currentToken.type) {
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_DOUBLE:
        case TOKEN_CHAR:
        case TOKEN_LONG:
        case TOKEN_SHORT:
        case TOKEN_SIGNED:
        case TOKEN_UNSIGNED:
        case TOKEN_VOID:
        case TOKEN_BOOL:
        case TOKEN_STRUCT:
        case TOKEN_UNION:
        case TOKEN_ENUM:
        case TOKEN_CONST:
        case TOKEN_VOLATILE:
        case TOKEN_RESTRICT:
        case TOKEN_INLINE:
        case TOKEN_STATIC:
        case TOKEN_EXTERN:
        case TOKEN_AUTO:
        case TOKEN_REGISTER:
            return true;
        default:
            break;
    }

    Parser temp = cloneParserWithFreshLexer(parser);
    ParsedType probe = parseTypeCtx(&temp, TYPECTX_Strict);
    bool result = false;
    if (probe.kind != TYPE_INVALID) {
        if (temp.currentToken.type == TOKEN_IDENTIFIER) {
            result = true;
        } else if (temp.currentToken.type == TOKEN_LPAREN) {
            result = true;
        }
    }

    parsedTypeFree(&probe);
    freeParserClone(&temp);
    return result;
}





bool looksLikeCastType(Parser* parser) {
    // Precondition: '(' was consumed by Pratt before nud('('),
    // so parser->currentToken is *after* '('.
    Parser probe = cloneParserWithFreshLexer(parser);

    // Parse a base type (primitive/typedef/struct/etc.)
    ParsedType t = parseTypeCtx(&probe, TYPECTX_Strict);
    if (t.kind == TYPE_INVALID) {
        parsedTypeFree(&t);
        freeParserClone(&probe);
        return false;
    }

    // Swallow abstract declarators: (*)(...), [N], (...)
    consumeAbstractDeclarator(&probe);

    // We must be at the closing ')'
    if (probe.currentToken.type != TOKEN_RPAREN) {
        parsedTypeFree(&t);
        freeParserClone(&probe);
        return false;
    }

    // (Optional sanity) after ')', should be a valid expr start
    advance(&probe); // consume ')'
    bool ok = isValidExpressionStart(probe.currentToken.type);

    parsedTypeFree(&t);
    parsedTypeFree(&t);
    freeParserClone(&probe);
    return ok;
}




bool looksLikeCompoundLiteral(Parser* parser) {
    // Pre: '(' already consumed by nud('('), so parser->currentToken is after '('
    Parser probe = cloneParserWithFreshLexer(parser);

    ParsedType t = parseTypeCtx(&probe, TYPECTX_Strict);
    if (t.kind == TYPE_INVALID) { parsedTypeFree(&t); freeParserClone(&probe); return false; }

    // Allow pointers/params/arrays after base type
    consumeAbstractDeclarator(&probe);

    // Must see ')'
    if (probe.currentToken.type != TOKEN_RPAREN) { parsedTypeFree(&t); freeParserClone(&probe); return false; }
    advance(&probe); // consume ')'

    // C99 compound literal: next token must be '{'
    bool ok = (probe.currentToken.type == TOKEN_LBRACE);
    parsedTypeFree(&t);
    parsedTypeFree(&t);
    freeParserClone(&probe);
    return ok;
}
