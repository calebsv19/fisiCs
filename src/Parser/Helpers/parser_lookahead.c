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
    if (parser->lexer) {
        printf("Lexer position:    %d\n", parser->lexer->position);
        printf("Lexer line:        %d\n", parser->lexer->line);
    }
    printf("===========================\n\n");
}


bool looksLikeTypeDeclaration(Parser* parser) {
    printf("\nLOOKAHEAD: Checking type declaration\n");

    Parser temp = cloneParserWithFreshLexer(parser);

    printf("  -> Initial token: %s (type %d) at line %d\n",
           temp.currentToken.value, temp.currentToken.type, temp.currentToken.line);

    // Step 1: Consume storage specifiers or modifiers
    while (isStorageSpecifier(temp.currentToken.type) || isModifierToken(temp.currentToken.type)) {
        printf("  -> Skipping modifier: %s (type %d)\n", temp.currentToken.value, temp.currentToken.type);
        advance(&temp);
    }

    // Step 2: Check for base type
    if (!(isPrimitiveTypeToken(temp.currentToken.type) || temp.currentToken.type == TOKEN_IDENTIFIER)) {
        printf("  -> Failed: No base type found. Got token: %s (type %d)\n",
               temp.currentToken.value, temp.currentToken.type);
        freeParserClone(&temp);
        return false;
    }

    printf("  -> Found type token: %s (type %d)\n",
           temp.currentToken.value, temp.currentToken.type);

    advance(&temp);

    // ✅ Step 3: Bail if it's a function pointer start: `(` comes next
    if (temp.currentToken.type == TOKEN_LPAREN) {
        printf("  -> Aborting: found '(' after type — likely a function pointer\n");
        freeParserClone(&temp);
        return false;
    }

    // Step 4: Handle pointer depth
    while (temp.currentToken.type == TOKEN_ASTERISK) {
        printf("  -> Found pointer '*'\n");
        advance(&temp);
    }

    // Step 5: Final check for identifier
    printf("  -> Final token before identifier check: %s (type %d)\n",
           temp.currentToken.value, temp.currentToken.type);

    bool result = (temp.currentToken.type == TOKEN_IDENTIFIER);

    if (result) {
        printf("  Lookahead result: valid type declaration (identifier = %s)\n", temp.currentToken.value);
    } else {
        printf("  Lookahead result: invalid — expected identifier, got %s (type %d)\n",
               temp.currentToken.value, temp.currentToken.type);
    }

    freeParserClone(&temp);
    return result;
}





bool looksLikeCastType(Parser* parser) {
    // Precondition: '(' was consumed by Pratt before nud('('),
    // so parser->currentToken is *after* '('.
    Parser probe = cloneParserWithFreshLexer(parser);

    // Parse a base type (primitive/typedef/struct/etc.)
    ParsedType t = parseType(&probe);
    if (t.kind == TYPE_INVALID) {
        freeParserClone(&probe);
        return false;
    }

    // Swallow abstract declarators: (*)(...), [N], (...)
    consumeAbstractDeclarator(&probe);

    // We must be at the closing ')'
    if (probe.currentToken.type != TOKEN_RPAREN) {
        freeParserClone(&probe);
        return false;
    }

    // (Optional sanity) after ')', should be a valid expr start
    advance(&probe); // consume ')'
    bool ok = isValidExpressionStart(probe.currentToken.type);

    freeParserClone(&probe);
    return ok;
}




bool looksLikeCompoundLiteral(Parser* parser) {
    // Pre: '(' already consumed by nud('('), so parser->currentToken is after '('
    Parser probe = cloneParserWithFreshLexer(parser);

    ParsedType t = parseType(&probe);
    if (t.kind == TYPE_INVALID) { freeParserClone(&probe); return false; }

    // Allow pointers/params/arrays after base type
    consumeAbstractDeclarator(&probe);

    // Must see ')'
    if (probe.currentToken.type != TOKEN_RPAREN) { freeParserClone(&probe); return false; }
    advance(&probe); // consume ')'

    // C99 compound literal: next token must be '{'
    bool ok = (probe.currentToken.type == TOKEN_LBRACE);
    freeParserClone(&probe);
    return ok;
}





bool looksLikeFunctionPointerDeclaration(Parser* parser) {
    printf("in look like fucntion pointer declaration. \n");
    Parser temp = cloneParserWithFreshLexer(parser);

    /* Skip a balanced param list quickly (we just need shape, not content) */
    int parens = 1;
    while (parens > 0 && temp.currentToken.type != TOKEN_EOF) {
        if (temp.currentToken.type == TOKEN_LPAREN) parens++;
        else if (temp.currentToken.type == TOKEN_RPAREN) parens--;
        advance(&temp);
    }
    if (parens != 0) { freeParserClone(&temp); return false; }

    /* Optional initializer: "= ..." — we don’t need to parse it, only accept it */
    if (temp.currentToken.type == TOKEN_ASSIGN) {
        advance(&temp);
        /* Accept a single expression-ish token sequence; as lookahead we can be loose.
           Stop at ';' or EOF. */
        while (temp.currentToken.type != TOKEN_SEMICOLON &&
               temp.currentToken.type != TOKEN_EOF) {
            advance(&temp);
        }
    }

    bool ok = (temp.currentToken.type == TOKEN_SEMICOLON);
    freeParserClone(&temp);
    return ok;
}
