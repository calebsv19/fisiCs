#include "parser_expr_pratt.h"
#include "parser_helpers.h"
#include "parser_decl.h"
#include "parser_array.h"
#include "parser_func.h"
#include "parser_lookahead.h"
#include "parser_expr.h" 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef ASTNode* (*NudFn)(Parser* parser, Token token);
typedef ASTNode* (*LedFn)(Parser* parser, ASTNode* left, Token token);

// Forward declarations
static ASTNode* nud(Parser* parser, Token token);
static ASTNode* led(Parser* parser, ASTNode* left, Token token);

// Operator precedence lookup
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
	case TOKEN_COMMA:
            return 0; // very low — right-associative
        default:
            return -1;
    }
}

int getTokenRightBindingPower(TokenType type) {
    int left = getTokenPrecedence(type);
    switch (type) {
        // Right-associative
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
            return left;  // Right-associative: use same value

        // Postfix ++/-- are left-associative
        case TOKEN_INCREMENT:
        case TOKEN_DECREMENT:
            return left + 1;

	case TOKEN_QUESTION:
	    return getTokenPrecedence(type);

	case TOKEN_COMMA:
	    return getTokenPrecedence(type) + 1;

        default:
            return left + 1;  // Left-associative by default
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

    while (getTokenPrecedence(parser->currentToken.type) >= minPrecedence) {
	printf("DEBUG: At token '%s' — minPrecedence: %d, token precedence: %d\n",
	       parser->currentToken.value, minPrecedence, getTokenPrecedence(parser->currentToken.type));

        Token op = parser->currentToken;
        int precedence = getTokenPrecedence(op.type);
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

        case TOKEN_LPAREN: {
		printf("  DEBUG: Entered nud() with TOKEN_LPAREN\n");
                if (looksLikeCastType(parser)) {
			printf("  DEBUG: Detected cast — calling parseCastExpressionPratt()\n");
                        return parseCastExpressionPratt(parser, true); // Use your full-featured cast parser
			printf("	RETURNED TRUE\n");
                } else {
			printf("  DEBUG: Not a cast — parsing grouped expression\n");
		}

                ASTNode* expr = parseExpressionPratt(parser, 0);  // Grouped expression
                if (parser->currentToken.type != TOKEN_RPAREN) {
                        printParseError("Expected ')' in grouping", parser);
                        return NULL;
                }
                advance(parser); // Consume ')'
                return expr;
        }

        case TOKEN_SIZEOF: {
            // TODO: implement sizeof type vs expr
            return parseSizeofExpression(parser);
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


        case TOKEN_LBRACKET:
            return parseArrayAccess(parser, left);

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
            ASTNode* arg = parseExpression(parser);
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

    if (parser->currentToken.type == TOKEN_LPAREN) {
	    advance(parser); // consume '('
    }

    ParsedType castType = parseType(parser);
    if (castType.kind == TYPE_INVALID) {
        printParseError("Invalid type in cast expression", parser);
        return NULL;
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' after cast type", parser);
        return NULL;
    }
    advance(parser); // consume ')'

    // Here's the fix: use Pratt version
    ASTNode* targetExpr = parseExpressionPratt(parser, 11);  // High precedence
    if (!targetExpr) {
        printParseError("Invalid expression after cast", parser);
        return NULL;
    }

    return createCastExpressionNode(castType, targetExpr);
}

