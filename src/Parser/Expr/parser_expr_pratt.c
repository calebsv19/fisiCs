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
#include "Utils/profiler.h"

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
        case TOKEN_COMMA: return 0;
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
    if (!isValidExpressionStart(parser->currentToken.type)) {
        printParseError("Unexpected token at start of expression", parser);
        return NULL;
    }
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

        case TOKEN_STRING: {
            size_t cap = 0;
            size_t len = 0;
            char* buf = NULL;
            bool sawWide = false;
            bool sawUtf8 = false;
            bool sawEscapedNarrow = false;
            const char* firstPayload = NULL;
            LiteralEncoding firstEnc = ast_literal_encoding(token.value ? token.value : "", &firstPayload);
            if (firstEnc == LIT_ENC_WIDE) {
                sawWide = true;
            } else if (firstEnc == LIT_ENC_UTF8) {
                sawUtf8 = true;
            } else if (token.value && strncmp(token.value, "N|", 2) == 0) {
                sawEscapedNarrow = true;
            }
            const char* first = firstPayload ? firstPayload : "";
            size_t firstLen = strlen(first);
            if (firstLen + 1 > cap) {
                size_t newCap = 64;
                while (newCap < firstLen + 1) newCap *= 2;
                buf = realloc(buf, newCap);
                cap = newCap;
            }
            if (buf) {
                memcpy(buf + len, first, firstLen);
                len += firstLen;
                buf[len] = '\0';
            }
            while (parser->currentToken.type == TOKEN_STRING) {
                const char* text = parser->currentToken.value ? parser->currentToken.value : "";
                const char* payload = NULL;
                LiteralEncoding enc = ast_literal_encoding(text, &payload);
                if (enc == LIT_ENC_WIDE) {
                    sawWide = true;
                } else if (enc == LIT_ENC_UTF8) {
                    sawUtf8 = true;
                } else if (strncmp(text, "N|", 2) == 0) {
                    sawEscapedNarrow = true;
                }
                const char* piece = payload ? payload : "";
                size_t tlen = strlen(piece);
                if (len + tlen + 1 > cap) {
                    size_t newCap = cap ? cap * 2 : 64;
                    while (newCap < len + tlen + 1) newCap *= 2;
                    char* next = realloc(buf, newCap);
                    if (!next) {
                        free(buf);
                        return NULL;
                    }
                    buf = next;
                    cap = newCap;
                }
                memcpy(buf + len, piece, tlen);
                len += tlen;
                buf[len] = '\0';
                advance(parser);
            }
            if (!buf) {
                buf = strdup("");
            }
            if (sawWide || sawUtf8 || sawEscapedNarrow) {
                const char* prefix = "N|";
                if (sawWide) {
                    prefix = "W|";
                } else if (sawUtf8) {
                    prefix = "U8|";
                }
                size_t plen = strlen(prefix);
                size_t total = plen + len + 1;
                char* merged = (char*)malloc(total);
                if (!merged) {
                    free(buf);
                    return NULL;
                }
                memcpy(merged, prefix, plen);
                memcpy(merged + plen, buf, len + 1);
                free(buf);
                buf = merged;
            }
            ASTNode* lit = createStringLiteralNode(buf);
            astNodeSetProvenance(lit, &token);
            free(buf);
            return lit;
        }

        case TOKEN_IDENTIFIER: {
                if (token.value && strcmp(token.value, "__extension__") == 0) {
                        return parseExpressionPratt(parser, 11);
                }
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
            ASTNode* node = createUnaryExprNode(getOperatorString(token.type), right, false);
            astNodeSetProvenance(node, &token);
            return node;
        }

        case TOKEN_BITWISE_AND: {  // prefix address-of
            ASTNode* right = parseExpressionPratt(parser, 11); // prefix-unary rbp
            ASTNode* node = createUnaryExprNode(getOperatorString(token.type), right, false);
            astNodeSetProvenance(node, &token);
            return node;
        }

	case TOKEN_LPAREN: {
	    PARSER_DEBUG_PRINTF("  DEBUG: Entered nud() with TOKEN_LPAREN\n");

    	// Reuse one strict type probe for cast-vs-compound-vs-group classification.
        ProfilerScope probeScope = profiler_begin("parser_pratt_lparen_type_probe");
        profiler_record_value("parser_count_pratt_lparen_type_probe", 1);
        ParenthesizedTypeFormKind typeForm = parserProbeParenthesizedTypeForm(parser);
        profiler_end(probeScope);

    if (typeForm == PARENTHESIZED_TYPE_FORM_COMPOUND_LITERAL) {
        return parseCompoundLiteralPratt(parser, /*alreadyConsumedLParen=*/true);
    }

    if (typeForm == PARENTHESIZED_TYPE_FORM_CAST) {
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
            return parseSizeofExpressionPratt(parser, &token);
        }
        case TOKEN_ALIGNOF: {
            return parseAlignofExpressionPratt(parser, &token);
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
	    // Allow comma expressions in the true arm (C grammar uses full "expression" here).
	    int rbp = getTokenRightBindingPower(type); // right-assoc for else arm
	    ASTNode* thenExpr = parseExpressionPratt(parser, -1);
	    if (!thenExpr) return NULL;	

	    if (parser->currentToken.type != TOKEN_COLON) {
	        printParseError("Expected ':' in ternary expression", parser);
	        return NULL;
	    }
	    advance(parser); // consume ':'
	
	    // Else arm is a conditional-expression; keep comma out (handled outside).
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
                    ASTNode** commaExprs = leftExpr->commaExpr.expressions;
                    items = realloc(commaExprs, sizeof(ASTNode*) * count);
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

    const char* calleeName = NULL;
    if (callee && callee->type == AST_IDENTIFIER) {
        calleeName = callee->valueNode.value;
    }
    bool isBuiltinVaArg = calleeName &&
                          (strcmp(calleeName, "__builtin_va_arg") == 0 ||
                           strcmp(calleeName, "va_arg") == 0);
    bool isBuiltinOffsetof = calleeName &&
                             (strcmp(calleeName, "__builtin_offsetof") == 0 ||
                              strcmp(calleeName, "offsetof") == 0);

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
            if (isBuiltinOffsetof && argCount == 0 &&
                parserLooksLikeTypeDeclarationAt(parser, PARSER_TYPE_DECL_SITE_BUILTIN_OFFSETOF)) {
                ParsedType ty = parseType(parser);
                arg = createParsedTypeNode(ty);
            } else if (isBuiltinOffsetof && argCount == 1) {
                if (parser->currentToken.type == TOKEN_IDENTIFIER) {
                    size_t cap = strlen(parser->currentToken.value) + 1;
                    char* path = (char*)malloc(cap);
                    if (!path) {
                        printf("Error: Memory allocation failed for __builtin_offsetof field path\n");
                        free(argList);
                        return NULL;
                    }
                    strcpy(path, parser->currentToken.value);
                    advance(parser);

                    while (parser->currentToken.type == TOKEN_DOT) {
                        advance(parser);
                        if (parser->currentToken.type != TOKEN_IDENTIFIER) {
                            printParseError("Expected field name after '.' in __builtin_offsetof", parser);
                            free(path);
                            free(argList);
                            return NULL;
                        }
                        size_t segLen = strlen(parser->currentToken.value);
                        size_t curLen = strlen(path);
                        size_t needed = curLen + 1 + segLen + 1;
                        if (needed > cap) {
                            char* grown = (char*)realloc(path, needed);
                            if (!grown) {
                                printf("Error: Memory allocation failed for __builtin_offsetof field path\n");
                                free(path);
                                free(argList);
                                return NULL;
                            }
                            path = grown;
                            cap = needed;
                        }
                        path[curLen] = '.';
                        memcpy(path + curLen + 1, parser->currentToken.value, segLen + 1);
                        advance(parser);
                    }

                    arg = createIdentifierNode(path);
                    free(path);
                } else {
                    printParseError("Expected field name in __builtin_offsetof", parser);
                    free(argList);
                    return NULL;
                }
            } else if (isBuiltinVaArg && argCount == 1 &&
                       parserLooksLikeTypeDeclarationAt(parser, PARSER_TYPE_DECL_SITE_BUILTIN_VA_ARG)) {
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
