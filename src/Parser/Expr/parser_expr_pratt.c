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

#include <stdlib.h>
#include <string.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef ASTNode* (*NudFn)(Parser* parser, Token token);
typedef ASTNode* (*LedFn)(Parser* parser, ASTNode* left, Token token);

// Forward declarations
static ASTNode* nud(Parser* parser, Token token);
static ASTNode* led(Parser* parser, ASTNode* left, Token token);
static ASTNode* parseGNUStatementExpression(Parser* parser);

// Base left-precedence values
int getTokenPrecedence(TokenType type) {
    switch (type) {
        case TOKEN_LOGICAL_OR: return 1;
        case TOKEN_LOGICAL_AND: return 2;
        case TOKEN_BITWISE_OR: return 3;
        case TOKEN_BITWISE_XOR: return 4;
        case TOKEN_BITWISE_AND: return 5;

        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL: return 6;

        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL: return 7;

        case TOKEN_LEFT_SHIFT:
        case TOKEN_RIGHT_SHIFT: return 8;

        case TOKEN_PLUS:
        case TOKEN_MINUS: return 9;

        case TOKEN_ASTERISK:
        case TOKEN_DIVIDE:
        case TOKEN_MODULO: return 10;

        case TOKEN_LPAREN:
        case TOKEN_LBRACKET:
        case TOKEN_DOT:
        case TOKEN_ARROW:  return 14;

        case TOKEN_INCREMENT:
        case TOKEN_DECREMENT: return 15;

        case TOKEN_ASSIGN:
        case TOKEN_PLUS_ASSIGN:
        case TOKEN_MINUS_ASSIGN:
        case TOKEN_MULT_ASSIGN:
        case TOKEN_DIV_ASSIGN:
        case TOKEN_MOD_ASSIGN:
        case TOKEN_LEFT_SHIFT_ASSIGN:
        case TOKEN_RIGHT_SHIFT_ASSIGN:
        case TOKEN_BITWISE_AND_ASSIGN:
        case TOKEN_BITWISE_OR_ASSIGN:
        case TOKEN_BITWISE_XOR_ASSIGN:
        case TOKEN_QUESTION:
            return 1;  // base precedence for right-assoc ops

        default:
            return -1;
    }
}

// Right-binding power: tweak for right-assoc
int getTokenRightBindingPower(TokenType type) {
    int p = getTokenPrecedence(type);
    switch (type) {
        case TOKEN_ASSIGN:
        case TOKEN_PLUS_ASSIGN:
        case TOKEN_MINUS_ASSIGN:
        case TOKEN_MULT_ASSIGN:
        case TOKEN_DIV_ASSIGN:
        case TOKEN_MOD_ASSIGN:
        case TOKEN_LEFT_SHIFT_ASSIGN:
        case TOKEN_RIGHT_SHIFT_ASSIGN:
        case TOKEN_BITWISE_AND_ASSIGN:
        case TOKEN_BITWISE_OR_ASSIGN:
        case TOKEN_BITWISE_XOR_ASSIGN:
        case TOKEN_QUESTION:
            return p - 1;  // right-associative
        default:
            return p;
    }
}



static ASTNode* parseGNUStatementExpression(Parser* parser) {
    if (parser->currentToken.type != TOKEN_LBRACE) {
        printParseError("Expected '{' to start statement expression", parser);
        return NULL;
    }
    ASTNode* block = parseBlock(parser);
    if (!block) {
        return NULL;
    }
    return createStatementExpressionNode(block);
}

// Main Pratt parser loop
ASTNode* parseExpressionPratt(Parser* parser, int minPrecedence) {
    Token token = parser->currentToken;
    advance(parser);

    ASTNode* left = nud(parser, token);
    if (!left) return NULL;

    PARSER_DEBUG_PRINTF("DEBUG: Starting Pratt parse loop with token: '%s' (precedence %d), minPrecedence=%d\n",
           parser->currentToken.value, getTokenPrecedence(parser->currentToken.type), minPrecedence);

    for (;;) {
        int prec = getTokenPrecedence(parser->currentToken.type);
        PARSER_DEBUG_PRINTF("DEBUG: At token '%s' — minPrecedence: %d, token precedence: %d\n",
               parser->currentToken.value, minPrecedence, prec);

        // Canonical stop: only continue when token precedence is strictly greater
        if (prec <= minPrecedence) break;

        Token op = parser->currentToken;
        advance(parser);

        left = led(parser, left, op);
        if (!left) return NULL;
    }

    return left;
}





// Null denotation: handles prefix/unary/start expressions
static ASTNode* nud(Parser* parser, Token token) {
    switch (token.type) {
        case TOKEN_NUMBER:
        case TOKEN_FLOAT_LITERAL: {
            ASTNode* lit = createNumberLiteralNode(token.value);
            astNodeSetProvenance(lit, &token);
            return lit;
        }

        case TOKEN_CHAR_LITERAL: {
            ASTNode* lit = createCharLiteralNode(token.value);
            astNodeSetProvenance(lit, &token);
            return lit;
        }

        case TOKEN_TRUE:
        case TOKEN_FALSE: {
            const char* boolValue = (token.type == TOKEN_TRUE) ? "1" : "0";
            ASTNode* lit = createNumberLiteralNode(boolValue);
            astNodeSetProvenance(lit, &token);
            return lit;
        }

        case TOKEN_STRING: {
            ASTNode* lit = createStringLiteralNode(token.value);
            astNodeSetProvenance(lit, &token);
            return lit;
        }

        case TOKEN_IDENTIFIER: {
                ASTNode* ident = createIdentifierNode(token.value);
                astNodeSetProvenance(ident, &token);

                // Peek: if the next token is '(', treat as a function call
                if (parser->currentToken.type == TOKEN_LPAREN) {
                        return parseFunctionCallPratt(parser, ident);  // Already in correct Pratt form
                }

                return ident;
        }

        case TOKEN_MINUS:
        case TOKEN_PLUS:
        case TOKEN_LOGICAL_NOT:
        case TOKEN_BITWISE_NOT:
        case TOKEN_INCREMENT:
        case TOKEN_DECREMENT:
        case TOKEN_ASTERISK: {
            ASTNode* right = parseExpressionPratt(parser, 11); // Higher than any binary
            return createUnaryExprNode(getOperatorString(token.type), right, false);
        }

        case TOKEN_BITWISE_AND: {  // prefix address-of
            ASTNode* right = parseExpressionPratt(parser, 11); // prefix-unary rbp
            return createUnaryExprNode(getOperatorString(token.type), right, false);
        }

	case TOKEN_LPAREN: {
	    PARSER_DEBUG_PRINTF("  DEBUG: Entered nud() with TOKEN_LPAREN\n");

    	// 1) Compound literal? (type) { ... }
    Parser probe = cloneParserWithFreshLexer(parser);
    ParsedType looked = parseTypeCtx(&probe, TYPECTX_Strict);
    bool treatAsCast = false;
    if (looked.kind != TYPE_INVALID) {
        consumeAbstractDeclarator(&probe);
        if (probe.currentToken.type == TOKEN_RPAREN) {
            treatAsCast = true;
        }
    }
    freeParserClone(&probe);

    if (looksLikeCompoundLiteral(parser)) {
        return parseCompoundLiteralPratt(parser, /*alreadyConsumedLParen=*/true);
    }

    if (treatAsCast) {
        return parseCastExpressionPratt(parser, /*alreadyConsumedLParen=*/true);
    }

    if (parser->currentToken.type == TOKEN_LBRACE) {
        if (!parser->enableStatementExpressions) {
            printParseError("GNU statement expressions are disabled (set ENABLE_GNU_STATEMENT_EXPRESSIONS=1)", parser);
            fprintf(stderr,
                    "GNU statement expressions are disabled (set ENABLE_GNU_STATEMENT_EXPRESSIONS=1)\n");
            return NULL;
        }
        ASTNode* stmtExpr = parseGNUStatementExpression(parser);
        if (!stmtExpr) {
            return NULL;
        }
        if (parser->currentToken.type != TOKEN_RPAREN) {
            printParseError("Expected ')' after statement expression", parser);
            return NULL;
        }
        advance(parser);
        return stmtExpr;
    }

    ASTNode* expr = parseExpressionPratt(parser, -1);
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' in grouping", parser);
        return NULL;
    }
    advance(parser);
    return expr;
	}
        case TOKEN_SIZEOF: {
            // TODO: implement sizeof type vs expr
            return parseSizeofExpressionPratt(parser);
        }
        case TOKEN_ALIGNOF: {
            return parseAlignofExpressionPratt(parser);
        }

        default:
            printParseError("Unexpected token at start of expression", parser);
            return NULL;
    }
}



// Left denotation: handles infix/postfix
static ASTNode* led(Parser* parser, ASTNode* left, Token token) {
    TokenType type = token.type;

    switch (type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_ASTERISK:
        case TOKEN_DIVIDE:
        case TOKEN_MODULO:
        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
        case TOKEN_LOGICAL_AND:
        case TOKEN_LOGICAL_OR:
        case TOKEN_BITWISE_AND:
        case TOKEN_BITWISE_OR:
        case TOKEN_BITWISE_XOR:
        case TOKEN_LEFT_SHIFT:
        case TOKEN_RIGHT_SHIFT:
            return createBinaryExprNode(getOperatorString(type), left,
                   parseExpressionPratt(parser, getTokenRightBindingPower(type)));

        case TOKEN_ASSIGN:
        case TOKEN_PLUS_ASSIGN:
        case TOKEN_MINUS_ASSIGN:
        case TOKEN_MULT_ASSIGN:
        case TOKEN_DIV_ASSIGN:
        case TOKEN_MOD_ASSIGN:
        case TOKEN_BITWISE_AND_ASSIGN:
        case TOKEN_BITWISE_OR_ASSIGN:
        case TOKEN_BITWISE_XOR_ASSIGN:
        case TOKEN_LEFT_SHIFT_ASSIGN:
        case TOKEN_RIGHT_SHIFT_ASSIGN:
            return createAssignmentNode(left,
                   parseExpressionPratt(parser, getTokenRightBindingPower(type)), type);

	case TOKEN_INCREMENT:
	case TOKEN_DECREMENT: {
	        // Postfix unary operator: left is the target expression
	        return createUnaryExprNode(getOperatorString(type), left, true); // true = isPostfix
	}


        case TOKEN_LPAREN:
	    return ledFunctionCall(parser, left);


	case TOKEN_LBRACKET: {
	    // index with tight floor; require ']'
	    ASTNode* index = parseExpressionPratt(parser, 0);
	    if (!index || parser->currentToken.type != TOKEN_RBRACKET) {
	        printParseError("Expected ']' after subscript expression", parser);
	        return NULL;
	    }
	    advance(parser); // consume ']'
	    ASTNode* node = createArrayAccessNode(left, index);
	    PARSER_DEBUG_PRINTF("DEBUG: After ARRAY_ACCESS, next token = '%s'\n", parser->currentToken.value);
	    return node; // allow chaining
	}

        case TOKEN_DOT:
        case TOKEN_ARROW: {
            if (parser->currentToken.type != TOKEN_IDENTIFIER) {
                printParseError("Expected identifier after member access", parser);
                return NULL;
            }
            char* field = parser->currentToken.value;
            advance(parser);
            return createMemberAccessNode(type, left, field);
        }

/*	case TOKEN_QUESTION: {
	        // Left is the condition
	        ASTNode* condition = left;
	
	        // Parse true branch
	        ASTNode* trueExpr = parseExpressionPratt(parser, 0);
	        if (!trueExpr) {
	            printParseError("Invalid expression after '?'", parser);
	            return NULL;
	        }
	
	        if (parser->currentToken.type != TOKEN_COLON) {
	            printParseError("Expected ':' in ternary expression", parser);
	            return NULL;
	        }
	        advance(parser);  // Consume ':'
	
	        // Parse false branch
	        ASTNode* falseExpr = parseExpressionPratt(parser, 0);
	        if (!falseExpr) {
	            printParseError("Invalid expression after ':' in ternary", parser);
	            return NULL;
	        }
	
	        return createTernaryExprNode(condition, trueExpr, falseExpr);
	}
*/
	case TOKEN_QUESTION: {
	    // parse the 'then' arm with rbp = p-1 so nested ?: binds inside
	    int rbp = getTokenRightBindingPower(type); // will be -1 with our table
	    ASTNode* thenExpr = parseExpressionPratt(parser, rbp);
	    if (!thenExpr) return NULL;	

	    if (parser->currentToken.type != TOKEN_COLON) {
	        printParseError("Expected ':' in ternary expression", parser);
	        return NULL;
	    }
	    advance(parser); // consume ':'
	
	    // parse the 'else' arm with the same rbp (right-associative)
	    ASTNode* elseExpr = parseExpressionPratt(parser, rbp);
	    if (!elseExpr) return NULL;
	
	    return createTernaryExprNode(left, thenExpr, elseExpr);
	}

	case TOKEN_COMMA: {
	        ASTNode* leftExpr = left;
	        ASTNode* rightExpr = parseExpressionPratt(parser, getTokenRightBindingPower(type));
        	if (!rightExpr) {
	            printParseError("Invalid expression after ','", parser);
	            return NULL;
	        }
	
	        // Flatten comma expression
	        ASTNode** items = NULL;
	        size_t count = 0;
	
	        if (leftExpr->type == AST_COMMA_EXPRESSION) {
	            // Copy existing items
	            count = leftExpr->commaExpr.exprCount + 1;
	            items = realloc(leftExpr->commaExpr.expressions, sizeof(ASTNode*) * count);
	            if (!items) {
	                printf("Error: Memory allocation failed during comma expansion\n");
	                return NULL;
	            }
	            items[count - 1] = rightExpr;
	
	            leftExpr->commaExpr.expressions = items;
	            leftExpr->commaExpr.exprCount = count;
	            return leftExpr;
	
	        } else {
	            // New comma expression from scratch
	            items = malloc(sizeof(ASTNode*) * 2);
	            if (!items) {
	                printf("Error: Memory allocation failed for new commaExpr\n");
	                return NULL;
	            }
	            items[0] = leftExpr;
	            items[1] = rightExpr;
	            return createCommaExprNode(items, 2);
	        }
	}


        default:
            printParseError("Unexpected token in expression", parser);
            return NULL;
    }
}






// ==========================================================
//		Helpers

// Consume a balanced (...) group (handles nesting). Returns false on mismatch.
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

// Consume zero-or-more abstract declarators after a base type.
// Accepts: ( ... ) groups, function param lists (...), and array suffixes [ ... ].
// We don't validate contents yet—just skip syntactically.
void consumeAbstractDeclarator(Parser* p) {
    for (;;) {
        if (p->currentToken.type == TOKEN_LPAREN) {
            // Could be (*), (params), or nested groups — just balance them
            consumeBalancedParens(p);
            continue;
        }
        if (p->currentToken.type == TOKEN_LBRACKET) {
            // [ constant-expression? ]
            advance(p); // '['
            // eat a simple constant/int literal or expression tokens until ']'
            // (keep it simple: skip until matching ']')
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

// Consume abstract declarators and record simple pointer/array layers onto `type`.
// Falls back to the lightweight skipper when patterns are more complex.
void consumeAbstractDeclaratorIntoType(Parser* p, ParsedType* type) {
    if (!type) {
        consumeAbstractDeclarator(p);
        return;
    }

    // Leading pointer stars
    while (p->currentToken.type == TOKEN_ASTERISK) {
        parsedTypeAppendPointer(type);
        advance(p);
    }

    bool sawComplex = false;
    while (p->currentToken.type == TOKEN_LPAREN) {
        // For now, treat parenthesised abstract declarators as opaque.
        sawComplex = true;
        consumeBalancedParens(p);
    }

    // Array suffixes
    while (p->currentToken.type == TOKEN_LBRACKET) {
        advance(p); // '['
        ASTNode* sizeExpr = NULL;
        if (p->currentToken.type != TOKEN_RBRACKET) {
            sizeExpr = parseExpressionPratt(p, 0);
        }
        if (p->currentToken.type != TOKEN_RBRACKET) {
            printParseError("Expected ']' after array declarator", p);
            return;
        }
        advance(p); // ']'

        parsedTypeAppendArray(type, sizeExpr, false);
        TypeDerivation* arr = parsedTypeGetMutableArrayDerivation(type, type->derivationCount - 1);
        if (arr && sizeExpr && sizeExpr->type == AST_NUMBER_LITERAL && sizeExpr->valueNode.value) {
            arr->as.array.hasConstantSize = true;
            arr->as.array.constantSize = atoll(sizeExpr->valueNode.value);
        }
    }

    if (sawComplex) {
        // Consume any remaining nested pieces without recording details.
        consumeAbstractDeclarator(p);
    }
}


bool looksLikeParenTypeName(Parser* parser) {
    if (parser->currentToken.type != TOKEN_LPAREN) return false;

    Parser probe = cloneParserWithFreshLexer(parser);
    advance(&probe); // consume '(' on probe

    ParsedType t = parseTypeCtx(&probe, TYPECTX_Strict);
    bool ok = (t.kind != TYPE_INVALID) && (probe.currentToken.type == TOKEN_RPAREN);
    freeParserClone(&probe);
    return ok;
}


ASTNode* ledFunctionCall(Parser* parser, ASTNode* callee) {
    // parse zero-or-more arguments until ')'
    ASTNode** args = NULL;
    size_t count = 0;

    if (parser->currentToken.type != TOKEN_RPAREN) {
        for (;;) {
            ASTNode* arg = parseExpressionPratt(parser, 0); // full expression
            if (!arg) return NULL;

            // push_back
            args = realloc(args, (count + 1) * sizeof(ASTNode*));
            args[count++] = arg;

            if (parser->currentToken.type != TOKEN_COMMA) break;
            advance(parser); // consume ','
        }
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' to close function call", parser);
        return NULL;
    }
    advance(parser); // consume ')'

    return createFunctionCallNode(callee, args, count);
}


ASTNode* parseFunctionCallPratt(Parser* parser, ASTNode* callee) {
    PARSER_DEBUG_PRINTF("DEBUG: Entering parseFunctionCallWithOpeningParenAlreadyConsumed() at line %d\n",
           parser->currentToken.line);

    if (parser->currentToken.type != TOKEN_LPAREN) {
        printParseError("Expected '(' to start function call", parser);
        return NULL;
    }
    advance(parser); // Consume '('

    const char* calleeName = (callee && callee->type == AST_IDENTIFIER) ? callee->valueNode.value : NULL;
    bool isBuiltinVaArg = calleeName &&
                          (strcmp(calleeName, "__builtin_va_arg") == 0 ||
                           strcmp(calleeName, "va_arg") == 0);

    size_t argCapacity = 4;
    size_t argCount = 0;
    ASTNode** argList = malloc(argCapacity * sizeof(ASTNode*));

    if (!argList) {
        printf("Error: Memory allocation failed for function arguments\n");
        return NULL;
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        while (1) {
            ASTNode* arg = NULL;
            if (isBuiltinVaArg && argCount == 1 && looksLikeTypeDeclaration(parser)) {
                ParsedType ty = parseType(parser);
                arg = createParsedTypeNode(ty);
            } else {
                arg = parseExpressionPratt(parser, 0);
                if (!arg) {
                    printf("Error: Invalid function argument at line %d\n", parser->currentToken.line);
                    free(argList);
                    return NULL;
                }
            }

            if (argCount >= argCapacity) {
                argCapacity *= 2;
                argList = realloc(argList, argCapacity * sizeof(ASTNode*));
                if (!argList) {
                    printf("Error: Memory reallocation failed for arguments\n");
                    return NULL;
                }
            }

            argList[argCount++] = arg;

            if (parser->currentToken.type == TOKEN_COMMA) {
                advance(parser);
                if (parser->currentToken.type == TOKEN_RPAREN) break;
            } else {
                break;
            }
        }
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' to close function call", parser);
        free(argList);
        return NULL;
    }

    advance(parser); // consume ')'
    PARSER_DEBUG_PRINTF("DEBUG: Function call parsed with %zu argument(s)\n", argCount);
    return createFunctionCallNode(callee, argList, argCount);
}


ASTNode* parseCastExpressionPratt(Parser* parser, bool alreadyConsumedLParen) {
    PARSER_DEBUG_PRINTF("DEBUG: [Pratt] Entering parseCastExpressionPratt() with token '%s' (line %d)\n",
           parser->currentToken.value, parser->currentToken.line);

    if (!alreadyConsumedLParen) {
        if (parser->currentToken.type != TOKEN_LPAREN) {
            printParseError("Expected '(' to start cast", parser);
            return NULL;
        }
        advance(parser); // '('
    }

    /* 1) Parse the type-specifier + pointer declarator (parseType handles stars now) */
    ParsedType castType = parseTypeCtx(parser, TYPECTX_Strict);
    if (castType.kind == TYPE_INVALID) {
        printParseError("Invalid type in cast expression", parser);
        return NULL;
    }

    // Accept abstract declarators after the base type: (*)(...), [N], (...)
    ParsedDeclarator abstractDecl;
    if (parserParseDeclarator(parser, &castType, false, false, &abstractDecl)) {
        ParsedType clone = parsedTypeClone(&abstractDecl.type);
        parserDeclaratorDestroy(&abstractDecl);
        parsedTypeFree(&castType);
        castType = clone;
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' after cast type", parser);
        return NULL;
    }
    advance(parser); // ')'

    // RHS as unary (tight prefix)
    ASTNode* targetExpr = parseExpressionPratt(parser, 11);
    if (!targetExpr) {
        printParseError("Invalid expression after cast", parser);
        return NULL;
    }

    return createCastExpressionNode(castType, targetExpr);
}



ASTNode* parseCompoundLiteralPratt(Parser* parser, bool alreadyConsumedLParen) {
    // '(' already consumed by nud(), unless explicitly told otherwise
    if (!alreadyConsumedLParen) {
        if (parser->currentToken.type != TOKEN_LPAREN) {
            printParseError("Expected '(' to start compound literal type", parser);
            return NULL;
        }
        advance(parser); // '('
    }

    // ( type [abstract-declarator] )
    ParsedType literalType = parseTypeCtx(parser, TYPECTX_Strict);
    if (literalType.kind == TYPE_INVALID) {
        printParseError("Invalid type in compound literal", parser);
        return NULL;
    }

    // allow things like int[], int(*)(void), etc., and record them on the type
    ParsedDeclarator abstractDecl;
    if (parserParseDeclarator(parser, &literalType, false, false, &abstractDecl)) {
        ParsedType clone = parsedTypeClone(&abstractDecl.type);
        parserDeclaratorDestroy(&abstractDecl);
        parsedTypeFree(&literalType);
        literalType = clone;
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' after compound literal type", parser);
        return NULL;
    }
    advance(parser); // ')'

    // { initializer-list }
    if (parser->currentToken.type != TOKEN_LBRACE) {
        printParseError("Expected '{' to start compound literal initializer", parser);
        return NULL;
    }
    advance(parser); // '{'

    DesignatedInit** items = NULL;
    size_t count = 0;

    while (parser->currentToken.type != TOKEN_RBRACE &&
           parser->currentToken.type != TOKEN_EOF) {

        DesignatedInit* di = NULL;

        if (parser->currentToken.type == TOKEN_DOT) {
            // .field = expr
            advance(parser); // '.'

            if (parser->currentToken.type != TOKEN_IDENTIFIER) {
                printParseError("Expected field name after '.' in designated initializer", parser);
                return NULL;
            }
            const char* fieldName = parser->currentToken.value; // token text from lexer
            advance(parser); // identifier

            if (parser->currentToken.type != TOKEN_ASSIGN) { // '='
                printParseError("Expected '=' after field designator", parser);
                return NULL;
            }
            advance(parser); // '='

            ASTNode* value = parseExpressionPratt(parser, 0);
            if (!value) return NULL;

            di = createDesignatedInit(fieldName, value);
            if (!di) return NULL;

        } else if (parser->currentToken.type == TOKEN_LBRACKET) {
            // [index] = expr
            advance(parser); // '['

            ASTNode* idx = parseExpressionPratt(parser, 0);
            if (!idx) return NULL;

            if (parser->currentToken.type != TOKEN_RBRACKET) {
                printParseError("Expected ']' after index designator", parser);
                return NULL;
            }
            advance(parser); // ']'

            if (parser->currentToken.type != TOKEN_ASSIGN) { // '='
                printParseError("Expected '=' after index designator", parser);
                return NULL;
            }
            advance(parser); // '='

            ASTNode* value = parseExpressionPratt(parser, 0);
            if (!value) return NULL;

            di = createSimpleInit(value);
            if (!di) return NULL;
            di->indexExpr = idx; // attach index designator

        } else {
            // positional initializer: just an expression
            ASTNode* value = parseExpressionPratt(parser, 0);
            if (!value) return NULL;

            di = createSimpleInit(value);
            if (!di) return NULL;
        }

        // append to items
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
            advance(parser); // ','
            // allow trailing comma before '}'
            if (parser->currentToken.type == TOKEN_RBRACE) break;
        }
    }

    if (parser->currentToken.type != TOKEN_RBRACE) {
        printParseError("Expected '}' to close compound literal initializer", parser);
        return NULL;
    }
    advance(parser); // '}'

    return createCompoundLiteralNode(literalType, items, count);
}







// Pratt-aware sizeof handler
// Precondition: 'sizeof' token was already consumed by parseExpressionPratt()
// so parser->currentToken is the token *after* 'sizeof'.
ASTNode* parseSizeofExpressionPratt(Parser* parser) {
    PARSER_DEBUG_PRINTF("DEBUG [Pratt]: entering parseSizeofExpressionPratt(), next token = '%s' (type %d)\n",
           parser->currentToken.value, parser->currentToken.type);

    if (parser->currentToken.type == TOKEN_LPAREN) {
        Parser temp = cloneParserWithFreshLexer(parser);
        advance(&temp); // '('

        ParsedType probeType = parseTypeCtx(&temp, TYPECTX_Strict);
        bool looksLikeType = (probeType.kind != TYPE_INVALID);
        if (looksLikeType) {
            consumeAbstractDeclarator(&temp);
            looksLikeType = (temp.currentToken.type == TOKEN_RPAREN);
        }
        freeParserClone(&temp);

        if (looksLikeType) {
            advance(parser);                      // '('
            ParsedType realType = parseTypeCtx(parser, TYPECTX_Strict);
            if (realType.kind == TYPE_INVALID) {
                printParseError("Invalid type in sizeof(type)", parser);
                return NULL;
            }
            consumeAbstractDeclarator(parser);
            if (parser->currentToken.type != TOKEN_RPAREN) {
                printParseError("Expected ')' after sizeof(type)", parser);
                return NULL;
            }
            advance(parser);                      // ')'
            return createSizeofNode(createParsedTypeNode(realType));
        }
        // else: fall through to expr form
    }

    ASTNode* operand = parseExpressionPratt(parser, 11);
    if (!operand) {
        printParseError("Invalid operand for sizeof", parser);
        return NULL;
    }
    return createSizeofNode(operand);
}

ASTNode* parseAlignofExpressionPratt(Parser* parser) {
    if (parser->currentToken.type == TOKEN_LPAREN) {
        Parser temp = cloneParserWithFreshLexer(parser);
        advance(&temp); // '('
        ParsedType probeType = parseTypeCtx(&temp, TYPECTX_Strict);
        bool looksLikeType = (probeType.kind != TYPE_INVALID);
        if (looksLikeType) {
            consumeAbstractDeclarator(&temp);
            looksLikeType = (temp.currentToken.type == TOKEN_RPAREN);
        }
        freeParserClone(&temp);

        if (looksLikeType) {
            advance(parser); // '('
            ParsedType realType = parseTypeCtx(parser, TYPECTX_Strict);
            if (realType.kind == TYPE_INVALID) {
                printParseError("Invalid type in alignof(type)", parser);
                return NULL;
            }
            consumeAbstractDeclarator(parser);
            if (parser->currentToken.type != TOKEN_RPAREN) {
                printParseError("Expected ')' after alignof(type)", parser);
                return NULL;
            }
            advance(parser); // ')'
            return createAlignofNode(createParsedTypeNode(realType));
        }
    }

    ASTNode* operand = parseExpressionPratt(parser, 11);
    if (!operand) {
        printParseError("Invalid operand for alignof", parser);
        return NULL;
    }
    return createAlignofNode(operand);
}
