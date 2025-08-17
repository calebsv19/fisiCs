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
    // We're assuming the first '(' is already consumed (nud() pre-advanced)
    Parser temp = cloneParserWithFreshLexer(parser);

    /* ── Step 1: Base type ─────────────────────────────────────────────── */
    if (isPrimitiveTypeToken(temp.currentToken.type)) {
        advance(&temp);
    } else if (temp.currentToken.type == TOKEN_STRUCT) {
        advance(&temp);
        if (temp.currentToken.type != TOKEN_IDENTIFIER) {
            freeParserClone(&temp);
            return false;
        }
        advance(&temp);
    } else if (temp.currentToken.type == TOKEN_IDENTIFIER) {
        if (!isKnownType(temp.currentToken.value)) {   // not a typedef / alias
            freeParserClone(&temp);
            return false;
        }
        advance(&temp);
    } else {
        freeParserClone(&temp);
        return false;
    }

    /* ── Step 2: Optional pointer depth (*, **, …) ─────────────────────── */
    while (temp.currentToken.type == TOKEN_ASTERISK) {
        advance(&temp);
    }

    /* ── Step 2b: Bail if next token starts a function-pointer tail ───── */
    /*  A simple cast ends with ')' right here.  
        If we see '(', we’re looking at something like
            (int (*)(float))expr
        or    (MyType (*)(void))expr
        which is a function-pointer declarator, not a plain cast type.      */
    if (temp.currentToken.type == TOKEN_LPAREN) {
        freeParserClone(&temp);
        return false;
    }

    /* ── Step 3: Expect closing ')' ────────────────────────────────────── */
    if (temp.currentToken.type != TOKEN_RPAREN) {
        freeParserClone(&temp);
        return false;
    }
    advance(&temp); // consume ')'

    /* ── Step 4: Cast must be followed by something that can start an expr */
    if (!isValidExpressionStart(temp.currentToken.type)) {
        freeParserClone(&temp);
        return false;
    }

    freeParserClone(&temp);
    return true;
}


bool looksLikeFunctionPointerDeclaration(Parser* parser) {
    Parser temp = cloneParserWithFreshLexer(parser);

    /* Skip storage + modifiers (same as parseType’s front matter) */
    while (isStorageSpecifier(temp.currentToken.type) || isModifierToken(temp.currentToken.type)) {
        advance(&temp);
    }

    /* Base type: primitive, struct/union <id>, or known typedef name */
    if (isPrimitiveTypeToken(temp.currentToken.type)) {
        advance(&temp);
    } else if (temp.currentToken.type == TOKEN_STRUCT || temp.currentToken.type == TOKEN_UNION) {
        TokenType tagTok = temp.currentToken.type;
        advance(&temp);
        if (temp.currentToken.type != TOKEN_IDENTIFIER) {
            freeParserClone(&temp);
            return false;
        }
        advance(&temp);
    } else if (temp.currentToken.type == TOKEN_IDENTIFIER) {
        if (!isKnownType(temp.currentToken.value)) {
            freeParserClone(&temp);
            return false;
        }
        advance(&temp);
    } else {
        freeParserClone(&temp);
        return false;
    }

    /* parseType() in real parse will consume leading '*'s; in lookahead we
       don’t need to — we just need to see the function-pointer tail now. */

    /* Expect '(' then one-or-more '*' then IDENT then ')' then '(' ... ')' */
    if (temp.currentToken.type != TOKEN_LPAREN) { freeParserClone(&temp); return false; }
    advance(&temp); /* '(' */

    if (temp.currentToken.type != TOKEN_ASTERISK) { freeParserClone(&temp); return false; }
    do { advance(&temp); } while (temp.currentToken.type == TOKEN_ASTERISK); /* allow (**name) */

    if (temp.currentToken.type != TOKEN_IDENTIFIER) { freeParserClone(&temp); return false; }
    advance(&temp); /* name */

    if (temp.currentToken.type != TOKEN_RPAREN) { freeParserClone(&temp); return false; }
    advance(&temp); /* ')' */

    if (temp.currentToken.type != TOKEN_LPAREN) { freeParserClone(&temp); return false; }
    advance(&temp); /* '(' */

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
