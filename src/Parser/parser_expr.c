#include "parser_expr.h"
#include "parser_helpers.h"
#include "parser_array.h"
#include "parser_lookahead.h"
#include "parser_func.h"
#include "parser_decl.h"

ASTNode* parseExpression(Parser* parser) {
    return parseCommaExpression(parser);  //  Ternary is the topmost operator in precedence
}

ASTNode* parseCommaExpression(Parser* parser) {
    ASTNode** exprList = malloc(sizeof(ASTNode*) * 4);
    size_t capacity = 4;
    size_t count = 0;   
    
    while (1) {
        ASTNode* expr = parseAssignmentExpression(parser);
        if (!expr) {
            printf("Error: Invalid expression in comma-separated list at line %d\n",
                        parser->currentToken.line);
            for (size_t i = 0; i < count; i++) free(exprList[i]);
            free(exprList);
            return NULL;   
        }
         
        if (count >= capacity) {
            capacity *= 2;
            exprList = realloc(exprList, sizeof(ASTNode*) * capacity);
        }
         
        exprList[count++] = expr;
        
        if (parser->currentToken.type != TOKEN_COMMA)
            break;
            
        advance(parser); // Consume ','
    }
     
    return createCommaExprNode(exprList, count);
}


ASTNode* parseAssignmentExpression(Parser* parser) {
    ASTNode* left = parseTernaryExpression(parser);
    if (!left) {
        printf("Error: Invalid left-hand side in assignment at line %d\n", 
				parser->currentToken.line);
        return NULL;
    }
            
    printf("DEBUG: Parsed LHS of assignment has AST type: %d\n", left->type);
         
    //  Only valid assignment targets
    bool isValidLHS = (
        left->type == AST_IDENTIFIER ||
        left->type == AST_POINTER_DEREFERENCE ||
        left->type == AST_ARRAY_ACCESS ||
        left->type == AST_DOT_ACCESS ||
        left->type == AST_POINTER_ACCESS
    );

    if (!isValidLHS) return left;
    
    if (isAssignmentOperator(parser->currentToken.type)) {
        TokenType assignOp = parser->currentToken.type;
        advance(parser); // consume the operator
            
        //  Right-associative assignment (supports chaining: a = b = c = 5)
        ASTNode* right = parseAssignmentExpression(parser);
        if (!right) {
            printf("Error: Invalid right-hand side in assignment at line %d\n", 
			parser->currentToken.line);
            return NULL;
        }
        
        //  For compound assignments, build binary RHS
        if (assignOp != TOKEN_ASSIGN) {
            char *opStr;
            switch (assignOp) {
                case TOKEN_PLUS_ASSIGN: opStr = "+"; break;
                case TOKEN_MINUS_ASSIGN: opStr = "-"; break;
                case TOKEN_MULT_ASSIGN: opStr = "*"; break;
                case TOKEN_DIV_ASSIGN: opStr = "/"; break;
                case TOKEN_MOD_ASSIGN: opStr = "%"; break;
                case TOKEN_BITWISE_AND_ASSIGN: opStr = "&"; break;
                case TOKEN_BITWISE_OR_ASSIGN: opStr = "|"; break;
                case TOKEN_BITWISE_XOR_ASSIGN: opStr = "^"; break;
                case TOKEN_LEFT_SHIFT_ASSIGN: opStr = "<<"; break;
                case TOKEN_RIGHT_SHIFT_ASSIGN: opStr = ">>"; break;
                default: opStr = "="; break;
            }
            right = createBinaryExprNode(opStr, createIdentifierNode(left->valueNode.value), right);
        }
         
        return createAssignmentNode(left, right, assignOp);
    }
     
    return left;
}


ASTNode* parseTernaryExpression(Parser* parser) {
    // First, parse a boolean expression (which may be the condition of a ternary operator)  
    ASTNode* condition = parseBooleanExpression(parser);  //  Parse boolean expressions first
    if (!condition) {
        return NULL;  // Return if no valid boolean expression is found
    }
    
    // Now, check for the ternary operator `?`
    if (parser->currentToken.type == TOKEN_QUESTION) {
        advance(parser); // Consume `?`
        
        // Parse the true branch of the ternary expression
        ASTNode* trueExpr = parseExpression(parser); // Use parseExpression for true expression
        if (!trueExpr) {
            printf("Error: Invalid true expression in ternary at line %d\n", 
			parser->currentToken.line);
            return NULL;
        }
        
        // Check for `:`, which is required after the true expression
        if (parser->currentToken.type != TOKEN_COLON) {
            printf("Error: Expected ':' in ternary at line %d\n", parser->currentToken.line);
            return NULL;
        }
        advance(parser); // Consume `:`
        
        // Parse the false branch of the ternary expression
        ASTNode* falseExpr = parseExpression(parser); //  Use parseExpression for false expression
        if (!falseExpr) {
            printf("Error: Invalid false expression in ternary at line %d\n",
                        parser->currentToken.line);
            return NULL;
        }
        
        // Create and return the ternary expression node
        return createTernaryExprNode(condition, trueExpr, falseExpr);
    }
    
    // If no `?`, return the parsed condition (it’s just a boolean expression without ternary)
    return condition;
}

ASTNode* parseBooleanExpression(Parser* parser) {
    ASTNode* left = parseComparisonExpression(parser);
    if (!left) {
        printf("Error: Invalid boolean expression at line %d\n", parser->currentToken.line);
        return NULL;
    }
     
    // Handle Logical Operators (&&, ||)
    while (parser->currentToken.type == TOKEN_LOGICAL_AND ||
           parser->currentToken.type == TOKEN_LOGICAL_OR) { 
        TokenType op = parser->currentToken.type;
        advance(parser);
        ASTNode* right = parseComparisonExpression(parser);
        if (!right) {
            printf("Error: Invalid right operand in boolean expression at line %d\n",
                        parser->currentToken.line);
            return NULL;
        }
        left = createBinaryExprNode(getOperatorString(op), left, right);
    }
     
    // **Handle Ternary Operator (`? :`)**
    if (parser->currentToken.type == TOKEN_QUESTION) {
        advance(parser); // Consume `?`
        ASTNode* trueExpr = parseExpression(parser);
        if (!trueExpr) {
            printf("Error: Invalid true expression in ternary at line %d\n", 
				parser->currentToken.line);
            return NULL;
        }
         
        if (parser->currentToken.type != TOKEN_COLON) {
            printf("Error: Expected ':' in ternary at line %d\n", parser->currentToken.line);
            return NULL;
        }
        advance(parser); // Consume `:`
        
        ASTNode* falseExpr = parseExpression(parser);
        if (!falseExpr) {
            printf("Error: Invalid false expression in ternary at line %d\n",
                        parser->currentToken.line);
            return NULL;
        }
         
        return createTernaryExprNode(left, trueExpr, falseExpr);
    }
     
    return left;
}

ASTNode* parseComparisonExpression(Parser* parser) {
    ASTNode* left = parseBitwiseOr(parser);
    if (!left) {
        printf("Error: Invalid comparison expression at line %d\n", parser->currentToken.line);
        return NULL;
    }
     
    while (parser->currentToken.type == TOKEN_LESS ||
           parser->currentToken.type == TOKEN_GREATER ||
           parser->currentToken.type == TOKEN_LESS_EQUAL ||
           parser->currentToken.type == TOKEN_GREATER_EQUAL ||
           parser->currentToken.type == TOKEN_EQUAL ||
           parser->currentToken.type == TOKEN_NOT_EQUAL) {
        TokenType op = parser->currentToken.type;
        advance(parser);
        ASTNode* right = parseBitwiseOr(parser);
        if (!right) {
            printf("Error: Invalid right operand in comparison at line %d\n",
                        parser->currentToken.line);
            return NULL;
        }
        left = createBinaryExprNode(getOperatorString(op), left, right);
    }
     
    return left;
}
 
ASTNode* parseBitwiseOr(Parser* parser) {
    ASTNode* left = parseBitwiseXor(parser);
    
    while (parser->currentToken.type == TOKEN_BITWISE_OR) {
        TokenType op = parser->currentToken.type;
        printf("DEBUG: Processing bitwise OR '|'\n");
        advance(parser);
        ASTNode* right = parseBitwiseXor(parser);
        left = createBinaryExprNode(getOperatorString(op), left, right);
    }
     
    return left;
}


// Handles bitwise XOR '^'
ASTNode* parseBitwiseXor(Parser* parser) {
    ASTNode* left = parseBitwiseAnd(parser);
    while (parser->currentToken.type == TOKEN_BITWISE_XOR) {
        TokenType op = parser->currentToken.type;
        advance(parser);
        ASTNode* right = parseBitwiseAnd(parser);
        left = createBinaryExprNode(getOperatorString(op), left, right); // Uses correct struct
    }
    return left;
}
 
// Handles bitwise AND '&'
ASTNode* parseBitwiseAnd(Parser* parser) {
    ASTNode* left = parseBitwiseShift(parser);
    while (parser->currentToken.type == TOKEN_BITWISE_AND) {
        TokenType op = parser->currentToken.type;
        advance(parser);
        ASTNode* right = parseBitwiseShift(parser);
        left = createBinaryExprNode(getOperatorString(op), left, right); // Uses correct struct
    }
    return left;
}
 
ASTNode* parseBitwiseShift(Parser* parser) {
    // Handles bitwise left/right shifts
    ASTNode* left = parseAdditionSubtraction(parser);
    while (parser->currentToken.type == TOKEN_LEFT_SHIFT ||
           parser->currentToken.type == TOKEN_RIGHT_SHIFT) {
        TokenType op = parser->currentToken.type;
        advance(parser);
        ASTNode* right = parseAdditionSubtraction(parser);
        left = createBinaryExprNode(getOperatorString(op), left, right); // Uses correct struct
    }
    return left;
}


ASTNode* parseAdditionSubtraction(Parser* parser) {
    // Handles addition and subtraction
    ASTNode* left = parseMultiplication(parser);
    while (parser->currentToken.type == TOKEN_PLUS ||
           parser->currentToken.type == TOKEN_MINUS) {
        TokenType op = parser->currentToken.type;
        advance(parser);
        ASTNode* right = parseMultiplication(parser);
        left = createBinaryExprNode(getOperatorString(op), left, right); // Uses correct struct
    }
    return left;
}
 
ASTNode* parseMultiplication(Parser* parser) {
    printf("DEBUG: Entering parseMult() with token '%s' (Type: %d) at line %d\n",
           parser->currentToken.value, parser->currentToken.type, parser->currentToken.line);
    // Handles multiplication, modulo, and division
    ASTNode* left = parseUnaryOrCast(parser);
    while (parser->currentToken.type == TOKEN_ASTERISK ||
           parser->currentToken.type == TOKEN_MODULO ||  
           parser->currentToken.type == TOKEN_DIVIDE) {  
        TokenType op = parser->currentToken.type;
        advance(parser);
        ASTNode* right = parseFactor(parser);
        left = createBinaryExprNode(getOperatorString(op), left, right); // Uses correct struct
    }
    return left;
}
 

ASTNode* parseUnaryOrCast(Parser* parser) {
    printf("DEBUG: Entering parseUnaryOrCast() with token '%s' (Type: %d) at line %d\n",
           parser->currentToken.value, parser->currentToken.type, parser->currentToken.line);

    if (parser->currentToken.type == TOKEN_LPAREN) {
        Token backupToken = parser->currentToken;
        if (looksLikeCastType(parser)) {
            printf("DEBUG: Looks like cast — calling parseCastExpression()\n");
            return parseCastExpression(parser);
        } else {
            printf("DEBUG: Not a cast — restoring '(' and continuing\n");
            parser->currentToken = backupToken;  // Reset state to treat as expression
        }
    }

    return parseFactor(parser);
}


ASTNode* parseFactor(Parser* parser) {
    printf("DEBUG: Entering parseFactor() with token '%s' (Type: %d) at line %d\n",
           parser->currentToken.value, parser->currentToken.type, parser->currentToken.line);
           
    // Handle prefix increment and decrement (++x, --x)
    if (parser->currentToken.type == TOKEN_INCREMENT ||
        parser->currentToken.type == TOKEN_DECREMENT) {
        TokenType op = parser->currentToken.type;
        advance(parser); // Consume '++' or '--' 
        ASTNode* operand = parseFactor(parser);  
        if (!operand) {
            printf("Error: Invalid operand for prefix '%s' at line %d\n",
                   getOperatorString(op), parser->currentToken.line);
            return NULL;
        }
        return createUnaryExprNode(getOperatorString(op), operand, false);
    }
     
    // Handle pointer dereference (*ptr, *--ptr, etc.)
    if (parser->currentToken.type == TOKEN_ASTERISK) {
        advance(parser); // Consume '*'
        ASTNode* operand = parseFactor(parser);
        if (!operand) {
            printf("Error: Invalid operand after '*' at line %d\n", parser->currentToken.line);
            return NULL;
        }
        return createPointerDereferenceNode(operand);
    }
     
    // Handle unary plus
    if (parser->currentToken.type == TOKEN_PLUS) {
        advance(parser);
        ASTNode* operand = parseFactor(parser);
        if (!operand) {
            printf("Error: Invalid operand for unary '+' at line %d\n", parser->currentToken.line);
            return NULL;
        }
        return createUnaryExprNode("+", operand, false);
    }

    // Handle negative number literals and unary minus
    if (parser->currentToken.type == TOKEN_MINUS) {   
        advance(parser);
        if (parser->currentToken.type == TOKEN_NUMBER) {
            char* negativeValue = malloc(strlen(parser->currentToken.value) + 2);
            sprintf(negativeValue, "-%s", parser->currentToken.value);
            ASTNode* negativeLiteral = createNumberLiteralNode(negativeValue);
            free(negativeValue);
            advance(parser);
            return negativeLiteral;
        } else {
            ASTNode* operand = parseFactor(parser);
            if (!operand) {
                printf("Error: Expected valid operand after '-' at line %d\n", 
				parser->currentToken.line);
                return NULL;
            }
            return createUnaryExprNode("-", operand, false);
        }
    }
     
    // Handle other unary operators (!, ~)
    if (parser->currentToken.type == TOKEN_LOGICAL_NOT ||
        parser->currentToken.type == TOKEN_BITWISE_NOT) {
        TokenType op = parser->currentToken.type;
        advance(parser);
        ASTNode* right = parseFactor(parser);
        return createUnaryExprNode(getOperatorString(op), right, false);
    }
     
    // Default: anything else should be parsed as a postfix expression
    return parsePostfixExpression(parser);
}


ASTNode* parseCastExpression(Parser* parser) {
    printf("DEBUG: Entering parseCastExpression() with token '%s' (line %d)\n",
           parser->currentToken.value, parser->currentToken.line);

    // Ensure we’re starting with a cast
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printParseError("Expected '(' at start of cast", parser);
        return NULL;
    }
    advance(parser); // consume '('

    // Parse the cast type inside the parentheses
    ParsedType castType = parseType(parser);
    if (castType.kind == TYPE_INVALID) {
        printParseError("Invalid type in cast expression", parser);
        return NULL;
    }

    // Match the closing ')'
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' after cast type", parser);
        return NULL;
    }
    advance(parser); // consume ')'

    // Parse the expression being casted
    ASTNode* targetExpr = parseUnaryOrCast(parser);
    if (!targetExpr) {
        printParseError("Invalid expression after cast", parser);
        return NULL;
    }

    return createCastExpressionNode(castType, targetExpr);
}




ASTNode* parsePostfixExpression(Parser* parser) {
    ASTNode* expr;
    
    // First: parse an identifier or primary expression   
    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        expr = createIdentifierNode(parser->currentToken.value);
        advance(parser); // consume identifier
    } else {
        expr = parsePrimary(parser);
    }
 
    if (!expr) return NULL;
        
    // Handle full postfix chaining: . -> [] ()
    while (true) {
        if (parser->currentToken.type == TOKEN_DOT ||
            parser->currentToken.type == TOKEN_ARROW) {
    
            TokenType op = parser->currentToken.type;
            advance(parser); // consume . or ->
        
            if (parser->currentToken.type != TOKEN_IDENTIFIER) {
                printf("Error: Expected identifier after '%s' at line %d\n",
                       op == TOKEN_DOT ? "." : "->", parser->currentToken.line);
                return NULL;
            }
        
            char* fieldName = parser->currentToken.value;
            advance(parser); // consume field name
            expr = createMemberAccessNode(op, expr, fieldName);
                
        } else if (parser->currentToken.type == TOKEN_LBRACKET) {
            expr = parseArrayAccess(parser, expr);  
        
        } else if (parser->currentToken.type == TOKEN_LPAREN) {
            expr = parseFunctionCall(parser, expr);
     
        } else {
            break;
        }
    }
    
    // Handle x++, y-- postfix unary ops
    while (parser->currentToken.type == TOKEN_INCREMENT ||
           parser->currentToken.type == TOKEN_DECREMENT) {
        TokenType op = parser->currentToken.type;
        advance(parser);  // Consume ++ or --
        expr = createUnaryExprNode(getOperatorString(op), expr, true);
    }
    
    return expr;
}


ASTNode* parsePrimary(Parser* parser) {
    printf("DEBUG: Entering parsePrimary() with token '%s' (Type: %d) at line %d\n",
           parser->currentToken.value, parser->currentToken.type, parser->currentToken.line);
           
    // Number & float literals
    if (parser->currentToken.type == TOKEN_FLOAT_LITERAL || parser->currentToken.type == TOKEN_NUMBER){
        ASTNode* node = createNumberLiteralNode(parser->currentToken.value);
        advance(parser);
        return node;
    }
     
    // Char literal
    if (parser->currentToken.type == TOKEN_CHAR_LITERAL) {
        ASTNode* node = createCharLiteralNode(parser->currentToken.value);
        advance(parser);
        return node;
    }
     
    // String literal
    if (parser->currentToken.type == TOKEN_STRING) {
        ASTNode* node = createStringLiteralNode(parser->currentToken.value);
        advance(parser);
        return node;
    }
     
    if (parser->currentToken.type == TOKEN_SIZEOF) {
        return parseSizeofExpression(parser);
    }
     
    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        ASTNode* node = createIdentifierNode(parser->currentToken.value);
        // Do not consume the identifier here — postfix handler will do it
        return node;
    }
     
    // Parenthesized expressions
    if (parser->currentToken.type == TOKEN_LPAREN) {
        advance(parser);
        ASTNode* expr = parseExpression(parser);
        if (parser->currentToken.type != TOKEN_RPAREN) {
            printf("Error: Expected ')' at line %d\n", parser->currentToken.line);
            return NULL;
        }
        advance(parser);  // Consume ')'
        return expr;
    }
     
    // Error fallback
    printf("Error: Unexpected token '%s' at line %d\n", parser->currentToken.value,
                parser->currentToken.line);
    return NULL;
}


ASTNode* parseSizeofExpression(Parser* parser) {
    printf("DEBUG: Entering parseSizeofExpression() at line %d\n", parser->currentToken.line);
        
    if (parser->currentToken.type != TOKEN_SIZEOF) {
        return NULL;
    }
            
    advance(parser);  // Consume 'sizeof'
            
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printParseError("Expected '(' after 'sizeof'", parser);
        return NULL;
    }
 
    advance(parser);  // Consume '('
            
    ASTNode* target = NULL;
         
    // Try parsing as a type  
    ParsedType type = parseType(parser);
    if (type.kind != TYPE_INVALID) {
        target = createParsedTypeNode(type);  // Type-based sizeof
    } else if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        target = createIdentifierNode(parser->currentToken.value);  // Variable-based sizeof
        advance(parser);  
    } else {
        printParseError("Invalid operand for 'sizeof'", parser);
        return NULL;
    }
    
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' after sizeof operand", parser);
        return NULL;
    }
    
    advance(parser);  // Consume ')'
    
    return createSizeofNode(target);
}


ASTNode* handleExpressionOrAssignment(Parser* parser) {
    // Always parse full expression (w/ identifiers, dot/arrow access, literals, function calls, etc$
    ASTNode* expr = parseExpression(parser);
         
    if (!expr) {
        printf("Error: Failed to parse expression at line %d\n", parser->currentToken.line);
        return NULL;
    }
    
    // Ensure expression ends in a semicolon
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printf("Error: expected ';' after expression at line %d\n", parser->currentToken.line);
        return NULL;
    }
     
    advance(parser); // Consume ';'
    
    //  Wrap comma expression as block-style sequence (multiple expressions at top level)
    if (expr->type == AST_COMMA_EXPRESSION) {
        ASTNode* blockNode = malloc(sizeof(ASTNode));
        blockNode->type = AST_BLOCK;
        blockNode->block.statementCount = expr->commaExpr.exprCount;
        blockNode->block.statements = expr->commaExpr.expressions;
        blockNode->nextStmt = NULL;
        return blockNode;
    }
            
    return expr;
} 
