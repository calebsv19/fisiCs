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

    Parser temp = cloneParserWithFreshLexer(parser);
    temp.currentToken = getNextToken(temp.lexer);  // consume '('

    if (isPrimitiveTypeToken(temp.currentToken.type)) {
        temp.currentToken = getNextToken(temp.lexer);
    } else if (temp.currentToken.type == TOKEN_STRUCT) {
        temp.currentToken = getNextToken(temp.lexer);
        if (temp.currentToken.type != TOKEN_IDENTIFIER) {
            freeParserClone(&temp);
            return false;
        }
        temp.currentToken = getNextToken(temp.lexer);
    } else if (temp.currentToken.type == TOKEN_IDENTIFIER) {
        temp.currentToken = getNextToken(temp.lexer);
    } else {
        freeParserClone(&temp);
        return false;
    }

    while (temp.currentToken.type == TOKEN_ASTERISK) {
        temp.currentToken = getNextToken(temp.lexer);
    }

    bool isValid = (temp.currentToken.type == TOKEN_RPAREN);
    freeParserClone(&temp);
    return isValid;
}


bool looksLikeFunctionPointerDeclaration(Parser* parser) {
    Parser temp = cloneParserWithFreshLexer(parser);

    while (isStorageSpecifier(temp.currentToken.type) || isModifierToken(temp.currentToken.type)) {
        temp.currentToken = getNextToken(temp.lexer);
    }

    if (!(isPrimitiveTypeToken(temp.currentToken.type) || temp.currentToken.type == TOKEN_IDENTIFIER)) {
        freeParserClone(&temp);
        return false;
    }

    temp.currentToken = getNextToken(temp.lexer);  // consume type

    while (temp.currentToken.type == TOKEN_ASTERISK) {
        temp.currentToken = getNextToken(temp.lexer);
    }

    if (temp.currentToken.type != TOKEN_LPAREN) {
        freeParserClone(&temp);
        return false;
    }
    temp.currentToken = getNextToken(temp.lexer);

    if (temp.currentToken.type != TOKEN_ASTERISK) {
        freeParserClone(&temp);
        return false;
    }
    temp.currentToken = getNextToken(temp.lexer);

    if (temp.currentToken.type != TOKEN_IDENTIFIER) {
        freeParserClone(&temp);
        return false;
    }
    temp.currentToken = getNextToken(temp.lexer);

    if (temp.currentToken.type != TOKEN_RPAREN) {
        freeParserClone(&temp);
        return false;
    }
    temp.currentToken = getNextToken(temp.lexer);

    bool isValid = (temp.currentToken.type == TOKEN_LPAREN);
    freeParserClone(&temp);
    return isValid;
}

