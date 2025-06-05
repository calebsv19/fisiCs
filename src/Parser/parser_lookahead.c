#include "parser_lookahead.h"
#include "parser_helpers.h"


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
    if (parser->currentToken.type != TOKEN_LPAREN) return false;

    printf("\nLOOKAHEAD: Checking cast expression\n");

    Parser temp = cloneParserWithFreshLexer(parser);
    advance(&temp);  // consume '('

    printf("  -> Token after '(': %s (type %d)\n", temp.currentToken.value, temp.currentToken.type);

    // Step 1: Check for base type
    if (isPrimitiveTypeToken(temp.currentToken.type)) {
        printf("  -> Found primitive type: %s\n", temp.currentToken.value);
        advance(&temp);
    } else if (temp.currentToken.type == TOKEN_STRUCT) {
        printf("  -> Found 'struct'\n");
        advance(&temp);

        if (temp.currentToken.type != TOKEN_IDENTIFIER) {
            printf("  -> Failed: Expected identifier after 'struct'\n");
            freeParserClone(&temp);
            return false;
        }

        printf("  -> Found struct identifier: %s\n", temp.currentToken.value);
        advance(&temp);
    } else if (temp.currentToken.type == TOKEN_IDENTIFIER) {
        printf("  -> Found user-defined type or typedef: %s\n", temp.currentToken.value);
        advance(&temp);
    } else {
        printf("  -> Failed: Not a valid type inside cast parentheses\n");
        freeParserClone(&temp);
        return false;
    }

    // Step 2: Handle optional pointer depth
    while (temp.currentToken.type == TOKEN_ASTERISK) {
        printf("  -> Found pointer '*'\n");
        advance(&temp);
    }

    // Step 3: Expect closing ')'
    if (temp.currentToken.type != TOKEN_RPAREN) {
        printf("  -> Failed: expected ')' after cast type, got '%s' (type %d)\n",
               temp.currentToken.value, temp.currentToken.type);
        freeParserClone(&temp);
        return false;
    }

    printf("  -> Lookahead result: valid cast expression\n");
    freeParserClone(&temp);
    return true;
}


bool looksLikeFunctionPointerDeclaration(Parser* parser) {
    printf("im hereeeeeeeeeeee\n");
    Parser temp = cloneParserWithFreshLexer(parser);

    // Skip storage/modifier tokens
    while (isStorageSpecifier(temp.currentToken.type) || isModifierToken(temp.currentToken.type)) {
        temp.currentToken = getNextToken(temp.lexer);
    }

    // Must be a valid base type (primitive or typedef/struct name)
    if (!(isPrimitiveTypeToken(temp.currentToken.type) || temp.currentToken.type == TOKEN_IDENTIFIER)) {
        freeParserClone(&temp);
        return false;
    }

    temp.currentToken = getNextToken(temp.lexer); // consume type

    // Now expect '(', then '*', then identifier, then ')', then '('
    if (temp.currentToken.type != TOKEN_LPAREN) {
        printf("  -> Failed: expected '(' after cast type, got '%s' (type %d)\n",
               temp.currentToken.value, temp.currentToken.type);
        freeParserClone(&temp);
        return false;
    }
    temp.currentToken = getNextToken(temp.lexer); // consume '('

    if (temp.currentToken.type != TOKEN_ASTERISK) {
	        printf("  -> Failed: expected '*' after '(' type, got '%s' (type %d)\n",
               temp.currentToken.value, temp.currentToken.type);
        freeParserClone(&temp);
        return false;
    }
    temp.currentToken = getNextToken(temp.lexer); // consume '*'

    if (temp.currentToken.type != TOKEN_IDENTIFIER) {
        printf("  -> Failed: expected identifier after '*' type, got '%s' (type %d)\n",
               temp.currentToken.value, temp.currentToken.type);
        freeParserClone(&temp);
        return false;
    }
    temp.currentToken = getNextToken(temp.lexer); // consume name

    if (temp.currentToken.type != TOKEN_RPAREN) {
        freeParserClone(&temp);
        return false;
    }
    temp.currentToken = getNextToken(temp.lexer); // consume ')'

    if (temp.currentToken.type != TOKEN_LPAREN) {
        freeParserClone(&temp);
        return false;
    }

    // You could also skip the parameter list by scanning till ')'
    int parens = 1;
    temp.currentToken = getNextToken(temp.lexer); // consume '('
    while (parens > 0 && temp.currentToken.type != TOKEN_EOF) {
        if (temp.currentToken.type == TOKEN_LPAREN) parens++;
        else if (temp.currentToken.type == TOKEN_RPAREN) parens--;
        temp.currentToken = getNextToken(temp.lexer);
    }

    if (parens != 0) {
        freeParserClone(&temp);
        return false;
    }

    bool isValid = (temp.currentToken.type == TOKEN_SEMICOLON);
    printf("MY booooool: %d\n", isValid);
    freeParserClone(&temp);
    return isValid;
}

