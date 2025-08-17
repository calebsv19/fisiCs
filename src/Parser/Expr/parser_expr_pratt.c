#include "parser_expr_pratt.h"
#include "parser_helpers.h"
#include "parser_decl.h"
#include "parser_array.h"
#include "parser_func.h"
#include "parser_lookahead.h"
#include "parser_expr.h" 
#include "AST/ast_node.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef ASTNode* (*NudFn)(Parser* parser, Token token);
typedef ASTNode* (*LedFn)(Parser* parser, ASTNode* left, Token token);

// Forward declarations
static ASTNode* nud(Parser* parser, Token token);
static ASTNode* led(Parser* parser, ASTNode* left, Token token);

// Operator precedence lookup
// Operator precedence lookup (higher number = tighter binding)
int getTokenPrecedence(TokenType type) {
    switch (type) {
        // Postfix / access (very high)
        case TOKEN_LPAREN:            // function call:   f(...)
        case TOKEN_LBRACKET:          // array subscript: a[...]
        case TOKEN_DOT:               // member access:   a.b
        case TOKEN_ARROW:             // ptr member:      a->b
            return 14;

        // Postfix ++/-- (highest among led ops you handle)
        case TOKEN_INCREMENT:         // a++
        case TOKEN_DECREMENT:         // a--
            return 13;

        // Multiplicative
        case TOKEN_ASTERISK:          // *
        case TOKEN_DIVIDE:            // /
        case TOKEN_MODULO:            // %
            return 10;

        // Additive
        case TOKEN_PLUS:              // +
        case TOKEN_MINUS:             // -
            return 9;

        // Shifts
        case TOKEN_LEFT_SHIFT:        // <<
        case TOKEN_RIGHT_SHIFT:       // >>
            return 8;

        // Relational
        case TOKEN_LESS:              // <
        case TOKEN_LESS_EQUAL:        // <=
        case TOKEN_GREATER:           // >
        case TOKEN_GREATER_EQUAL:     // >=
            return 7;

        // Equality
        case TOKEN_EQUAL:             // ==
        case TOKEN_NOT_EQUAL:         // !=
            return 6;

        // Bitwise
        case TOKEN_BITWISE_AND:       // &   (binary; unary & handled in nud)
            return 5;
        case TOKEN_BITWISE_XOR:       // ^
            return 4;
        case TOKEN_BITWISE_OR:        // |
            return 3;

        // Logical
        case TOKEN_LOGICAL_AND:       // &&
            return 2;
        case TOKEN_LOGICAL_OR:        // ||
            return 1;

        // Conditional / assignments / comma (lowest; right-assoc handled in parser logic)
        case TOKEN_QUESTION:          // ?:  (handled specially in led/nud)
        case TOKEN_ASSIGN:            // =
        case TOKEN_PLUS_ASSIGN:       // +=
        case TOKEN_MINUS_ASSIGN:      // -=
        case TOKEN_MULT_ASSIGN:       // *=
        case TOKEN_DIV_ASSIGN:        // /=
        case TOKEN_MOD_ASSIGN:        // %=
        case TOKEN_LEFT_SHIFT_ASSIGN: // <<=
        case TOKEN_RIGHT_SHIFT_ASSIGN:// >>=
        case TOKEN_BITWISE_AND_ASSIGN:// &=
        case TOKEN_BITWISE_OR_ASSIGN: // |=
        case TOKEN_BITWISE_XOR_ASSIGN:// ^=
        case TOKEN_COMMA:             // ,
            return 0;

        default:
            return -1; // not an infix/postfix operator for Pratt
    }
}

int getTokenRightBindingPower(TokenType type) {
    int p = getTokenPrecedence(type);

    switch (type) {
        // Right-associative operators
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
            return p - 1;

        // If you route '?' here, make it right-assoc as well.
        // (':' is handled inside your led for ternary.)
        case TOKEN_QUESTION:
            return p - 1;

        // Everything else is left-associative
        // (includes comma, logical/bitwise, relational, shifts, +/-, */,%, etc.)
        default:
            return p;
    }
}


// Main Pratt parser loop
ASTNode* parseExpressionPratt(Parser* parser, int minPrecedence) {
    Token token = parser->currentToken;
    advance(parser);

    ASTNode* left = nud(parser, token);
    if (!left) return NULL;

    printf("DEBUG: Starting Pratt parse loop with token: '%s' (precedence %d)\n",
       parser->currentToken.value, getTokenPrecedence(parser->currentToken.type));

    for (;;) {
	    int prec = getTokenPrecedence(parser->currentToken.type);
	    if (prec <= minPrecedence) break;  // strict binding: only operators with higher prec may bind
	
	    printf("DEBUG: At token '%s' — minPrecedence: %d, token precedence: %d\n",
	           parser->currentToken.value, minPrecedence, prec);
	
	    Token op = parser->currentToken;
	    advance(parser);  // consume operator
	
	    left = led(parser, left, op);
	    if (!left) return NULL;
     }
	
     return left;
}





// Null denotation: handles prefix/unary/start expressions
static ASTNode* nud(Parser* parser, Token token) {
    switch (token.type) {
        case TOKEN_NUMBER:
        case TOKEN_FLOAT_LITERAL:
            return createNumberLiteralNode(token.value);

        case TOKEN_CHAR_LITERAL:
            return createCharLiteralNode(token.value);

        case TOKEN_STRING:
            return createStringLiteralNode(token.value);

        case TOKEN_IDENTIFIER: {
                ASTNode* ident = createIdentifierNode(token.value);

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

	case TOKEN_BITWISE_AND: {  // NEW: address-of
	    ASTNode* right = parseExpressionPratt(parser, 11);
	    return createUnaryExprNode(getOperatorString(token.type), right, false);
	}

        case TOKEN_LPAREN: {
                printf("  DEBUG: Entered nud() with TOKEN_LPAREN\n");

                // Speculative parse: try cast first
                Parser snapshot = cloneParserWithFreshLexer(parser);
                ASTNode* tryCast = parseCastExpressionPratt(&snapshot, true);
                if (tryCast) {
                        printf("  DEBUG: Detected cast — committing parseCastExpressionPratt()\n");
                        *parser = snapshot; // commit snapshot
                        return tryCast;
                }
                freeParserClone(&snapshot);

                printf("  DEBUG: Not a cast — parsing grouped expression\n");
                ASTNode* expr = parseExpressionPratt(parser, 0);  // Grouped expression
                if (parser->currentToken.type != TOKEN_RPAREN) {
                        printParseError("Expected ')' in grouping", parser);
                        return NULL;
                }
                advance(parser); // Consume ')'
                return expr;
        }

        case TOKEN_SIZEOF: {
	    return parseSizeofExpressionPratt(parser);
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
	    return parseFunctionCallPratt(parser, left);


	case TOKEN_LBRACKET: {
	    // left is the base expression already parsed by Pratt
	    left = parseArrayAccess(parser, left);
	    return left;
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

	case TOKEN_QUESTION: {
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

ASTNode* parseFunctionCallPratt(Parser* parser, ASTNode* callee) {
    printf("DEBUG: Entering parseFunctionCallWithOpeningParenAlreadyConsumed() at line %d\n",
           parser->currentToken.line);

    if (parser->currentToken.type != TOKEN_LPAREN) {
        printParseError("Expected '(' to start function call", parser);
        return NULL;
    }
    advance(parser); // Consume '('

    size_t argCapacity = 4;
    size_t argCount = 0;
    ASTNode** argList = malloc(argCapacity * sizeof(ASTNode*));

    if (!argList) {
        printf("Error: Memory allocation failed for function arguments\n");
        return NULL;
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        while (1) {
            ASTNode* arg = parseExpressionPratt(parser, 0);
            if (!arg) {
                printf("Error: Invalid function argument at line %d\n", parser->currentToken.line);
                free(argList);
                return NULL;
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
    printf("DEBUG: Function call parsed with %zu argument(s)\n", argCount);
    return createFunctionCallNode(callee, argList, argCount);
}


ASTNode* parseCastExpressionPratt(Parser* parser, bool alreadyConsumedLParen) {
    printf("DEBUG: [Pratt] Entering parseCastExpressionPratt() with token '%s' (line %d)\n",
           parser->currentToken.value, parser->currentToken.line);

    if (!alreadyConsumedLParen) {
        if (parser->currentToken.type != TOKEN_LPAREN) {
            printParseError("Expected '(' to start cast", parser);
            return NULL;
        }
        advance(parser); /* consume '(' */
    }

    /* 1) Parse the type-specifier + pointer declarator (parseType handles stars now) */
    ParsedType castType = parseType(parser);
    if (castType.kind == TYPE_INVALID) {
        printParseError("Invalid type in cast expression", parser);
        return NULL;
    }

    /* 2) Optional function-pointer abstract declarator: ( * [ident]? ) ( params ) 
           Note: parseType already handled top-level '*'s in the declarator.
           Here we only detect and parse the '(* ...)( ... )' tail, and mark the type as FP. */
    if (parser->currentToken.type == TOKEN_LPAREN) {
        Parser snap = cloneParserWithFreshLexer(parser);
        bool matched = false;
        int starsFromParens = 0;

        /* local scratch to collect params; copied into castType at the end */
        size_t cap = 4, cnt = 0;
        ParsedType* params = (ParsedType*)malloc(cap * sizeof(ParsedType));
        if (!params) { cap = 0; } /* OOM: we'll still parse but won't store params */

        advance(&snap); /* consume '(' before '*' */

        if (snap.currentToken.type == TOKEN_ASTERISK) {
            /* require at least one '*' */
            do {
                ++starsFromParens;
                advance(&snap);
                /* while (isTypeQualifier(snap.currentToken.type)) advance(&snap); */
            } while (snap.currentToken.type == TOKEN_ASTERISK);

            /* optional name inside the '(*name*)' — casts don't need it */
            if (snap.currentToken.type == TOKEN_IDENTIFIER) {
                advance(&snap);
            }

            if (snap.currentToken.type == TOKEN_RPAREN) {
                advance(&snap);

                if (snap.currentToken.type == TOKEN_LPAREN) {
                    advance(&snap);

                    /* param-type-list: either 'void' or comma-separated types */
                    bool onlyVoid = false;
                    if (snap.currentToken.type != TOKEN_RPAREN) {
                        while (1) {
                            if (snap.currentToken.type == TOKEN_VOID) {
                                advance(&snap);
                                if (snap.currentToken.type == TOKEN_RPAREN) {
                                    onlyVoid = true;
                                }
                                if (onlyVoid) break;
                            }

                            /* Parameter type: parseType now consumes its own '*' declarator */
                            ParsedType p = parseType(&snap);
                            if (p.kind == TYPE_INVALID) {
                                printParseError("Invalid parameter type in function pointer cast", &snap);
                                if (params) free(params);
                                freeParserClone(&snap);
                                return NULL;
                            }

                            /* Optional parameter name (ignored). Accept only if followed by ',' or ')' */
                            if (snap.currentToken.type == TOKEN_IDENTIFIER) {
                                TokenType look = snap.nextToken.type;
                                if (look == TOKEN_COMMA || look == TOKEN_RPAREN) {
                                    advance(&snap);
                                }
                            }

                            /* store param */
                            if (!onlyVoid) {
                                if (cnt == cap) {
                                    size_t ncap = cap ? cap * 2 : 4;
                                    ParsedType* nbuf = (ParsedType*)realloc(params, ncap * sizeof(ParsedType));
                                    if (nbuf) { params = nbuf; cap = ncap; }
                                }
                                if (cnt < cap) params[cnt++] = p;
                            }

                            if (snap.currentToken.type == TOKEN_COMMA) {
                                advance(&snap);
                                continue;
                            }
                            break;
                        }
                    }

                    if (snap.currentToken.type != TOKEN_RPAREN) {
                        printParseError("Expected ')' to close function pointer parameter list", &snap);
                        if (params) free(params);
                        freeParserClone(&snap);
                        return NULL;
                    }
                    advance(&snap);

                    matched = true;

                    /* Commit parse state */
                    *parser = snap;

                    /* Mark function-pointer + store params */
                    if (onlyVoid) {
                        parsedTypeSetFunctionPointer(&castType, 0, NULL);
                    } else {
                        parsedTypeSetFunctionPointer(&castType, cnt, params);
                    }
                }
            }
        }

        if (!matched) {
            freeParserClone(&snap);
            if (params) free(params);
        } else {
            if (params) free(params);
        }
    }

    /* 3) Expect ')' to end the type-name */
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' after cast type", parser);
        return NULL;
    }
    advance(parser); /* consume ')' */

    /* 4) Parse the target expression at high precedence so the cast binds tightly */
    ASTNode* targetExpr = parseExpressionPratt(parser, 11);
    if (!targetExpr) {
        printParseError("Invalid expression after cast", parser);
        return NULL;
    }

    return createCastExpressionNode(castType, targetExpr);
}






// 		HELPERS

ASTNode* parseSizeofExpressionPratt(Parser* parser) {
    // assuming TOKEN_SIZEOF has already been consumed by nud before calling this
    if (parser->currentToken.type == TOKEN_LPAREN) {
        // Speculative parse of a type-name: ( type )
        Parser snap = cloneParserWithFreshLexer(parser);
        advance(&snap); // '('

        ParsedType t = parseType(&snap);
        if (t.kind != TYPE_INVALID && snap.currentToken.type == TOKEN_RPAREN) {
            // Commit the speculative parse
            *parser = snap;
            advance(parser); // consume ')'

            // Build a type node and wrap in sizeof
            ASTNode* typeNode = createParsedTypeNode(t);   // <-- available in your AST
            return createSizeofNode(typeNode);             // <-- single sizeof node kind
        }
        freeParserClone(&snap);
        // fall through to sizeof <expr>
    }

    // sizeof <expr>
    ASTNode* e = parseExpressionPratt(parser, 11);
    if (!e) {
        printParseError("Invalid expression after sizeof", parser);
        return NULL;
    }
    return createSizeofNode(e); // reuse the same node kind
}
