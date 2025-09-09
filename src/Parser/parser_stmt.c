#include "parser_helpers.h"
#include "parser_stmt.h"
#include "parser_main.h"
#include "parser_decl.h"
#include "parser_switch.h"
#include "Parser/Expr/parser_expr.h"
#include "Parser/Expr/parser_expr_pratt.h"


ASTNode* handleControlStatements(Parser* parser) {
    // === Type Definitions & Declarations === 
    if (parser->currentToken.type == TOKEN_STRUCT) {
        return handleStructStatements(parser);
    }
    if (parser->currentToken.type == TOKEN_UNION)
        return parseUnionDefinition(parser);
    if (parser->currentToken.type == TOKEN_ENUM)
        return parseEnumDefinition(parser);
    if (parser->currentToken.type == TOKEN_TYPEDEF)
        return parseTypedef(parser);
    
    // === Loops and tests  ===
    if (parser->currentToken.type == TOKEN_IF)
        return parseIfStatement(parser);
    if (parser->currentToken.type == TOKEN_WHILE)
        return parseWhileLoop(parser);
    if (parser->currentToken.type == TOKEN_DO)
        return parseDoWhileLoop(parser);
    if (parser->currentToken.type == TOKEN_FOR)
        return parseForLoop(parser);
    if (parser->currentToken.type == TOKEN_SWITCH)
        return parseSwitchStatement(parser);
    
    // === Flow Control ===
    if (parser->currentToken.type == TOKEN_GOTO)
        return parseGotoStatement(parser);
    if (parser->currentToken.type == TOKEN_RETURN)
        return parseReturnStatement(parser);
    if (parser->currentToken.type == TOKEN_BREAK)
        return parseBreakStatement(parser);
    if (parser->currentToken.type == TOKEN_CONTINUE)
        return parseContinueStatement(parser);
    if (parser->currentToken.type == TOKEN_ASM) {
        return parseAsmStatement(parser);
    }
     
     
    return NULL; // No matching control keyword
}




ASTNode* parseAssignment(Parser* parser) {
    // Ensure we are assigning to a valid identifier
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printf("Error: expected variable name at line %d\n", parser->currentToken.line);
        return NULL;
    }
    
    ASTNode* idNode = createIdentifierNode(parser->currentToken.value);
    advance(parser); // Consume identifier
    
    // Handle assignment operators: '=', '+=', '-=', '*=', '/=', '%=', '&=', '|=', '^=', '<<=', '>>='
    TokenType assignOp = parser->currentToken.type;
    if (assignOp == TOKEN_ASSIGN || assignOp == TOKEN_PLUS_ASSIGN ||
        assignOp == TOKEN_MINUS_ASSIGN || assignOp == TOKEN_MULT_ASSIGN ||
        assignOp == TOKEN_DIV_ASSIGN || assignOp == TOKEN_MOD_ASSIGN ||
        assignOp == TOKEN_BITWISE_AND_ASSIGN || assignOp == TOKEN_BITWISE_OR_ASSIGN ||
        assignOp == TOKEN_BITWISE_XOR_ASSIGN || assignOp == TOKEN_LEFT_SHIFT_ASSIGN ||
        assignOp == TOKEN_RIGHT_SHIFT_ASSIGN) {
     
        advance(parser); // Consume assignment operator
    } else {
        printf("Error: expected assignment operator at line %d\n", parser->currentToken.line);
        return NULL;
    }
    
    ASTNode* expr = parseExpression(parser);
             
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printf("Error: expected ';' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume ';'
            
    return createAssignmentNode(idNode, expr, assignOp);
}


// tests and loops
ASTNode* parseIfStatement(Parser* parser) {
    if (parser->currentToken.type != TOKEN_IF) {
        printf("Error: expected 'if' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume 'if'
         
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printf("Error: expected '(' after 'if' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume '('
        
    ASTNode* condition = parseBooleanExpression(parser);
 
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printf("Error: expected ')' after condition at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume ')'
        
    ASTNode* thenBody = NULL;
    if (parser->currentToken.type == TOKEN_LBRACE) {
        thenBody = parseBlock(parser);
    } else {
        thenBody = parseStatement(parser);
    }
     
    if (!thenBody) {
        printf("Error: Invalid body in 'if' statement at line %d\n", parser->currentToken.line);
        return NULL;
    }
        
    ASTNode* elseBody = NULL;
    if (parser->currentToken.type == TOKEN_ELSE) {
        advance(parser); // Consume 'else'
        if (parser->currentToken.type == TOKEN_IF) {
            // Handle 'else if' as an embedded if statement
            elseBody = parseIfStatement(parser);
        } else {
            elseBody = (parser->currentToken.type == TOKEN_LBRACE) ? parseBlock(parser) :
                                parseStatement(parser);
        }
    
        if (!elseBody) {
            printf("Error: Invalid body in 'else' statement at line %d\n", 
				parser->currentToken.line);
            return NULL;
        }
    }
    
    if (elseBody) {
        return createIfElseStatementNode(condition, thenBody, elseBody);
    } else {
        return createIfStatementNode(condition, thenBody);
    }   
}

ASTNode* parseWhileLoop(Parser* parser) {
    if (parser->currentToken.type != TOKEN_WHILE) {
        printf("Error: expected 'while' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume 'while'
    
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printf("Error: expected '(' after 'while' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume '('
    
    ASTNode* condition = parseBooleanExpression(parser);
    if (!condition) {
        printf("Error: Invalid condition in 'while' loop at line %d\n", parser->currentToken.line);
        return NULL;
    }
     
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printf("Error: expected ')' after while condition at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume ')'
    
    // Parse loop body: Prefer `{}` blocks but allow single statements
    ASTNode* body;
    if (parser->currentToken.type == TOKEN_LBRACE) {
        body = parseBlock(parser); // Parse full block `{ ... }`
    } else {
        body = parseStatement(parser); // Parse single statement
    }
     
    if (!body) {
        printf("Error: Invalid body in 'while' loop at line %d\n", parser->currentToken.line);
        return NULL;
    }
     
    return createWhileLoopNode(condition, body, false);
}

ASTNode* parseDoWhileLoop(Parser* parser) {
    if (parser->currentToken.type != TOKEN_DO) {
        printf("Error: expected 'do' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume 'do'
    
    // Parse loop body: Prefer `{}` blocks but allow single statements
    ASTNode* body;
    if (parser->currentToken.type == TOKEN_LBRACE) {
        body = parseBlock(parser); // Parse full block `{ ... }`
    } else {
        body = parseStatement(parser); // Parse single statement
    }
     
    if (!body) {
        printf("Error: Invalid body in 'do-while' loop at line %d\n", parser->currentToken.line);
        return NULL;
    }
     
    // Ensure 'while' follows
    if (parser->currentToken.type != TOKEN_WHILE) {
        printf("Error: expected 'while' after 'do' block at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume 'while'
    
    // Ensure '(' follows 'while'
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printf("Error: expected '(' after 'while' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume '('
    
    // Parse loop condition
    ASTNode* condition = parseBooleanExpression(parser);
    if (!condition) {
        printf("Error: Invalid condition in 'do-while' loop at line %d\n", 
				parser->currentToken.line);
        return NULL;
    }
     
    // Ensure ')' follows condition
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printf("Error: expected ')' after condition at line %d\n", 
			parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume ')'
    
    // Ensure ';' follows ')'
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printf("Error: expected ';' after 'do-while' statement at line %d\n", 
			parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume ';'
    
    return createWhileLoopNode(condition, body, true); // `true` indicates a `do-while` loop
}

ASTNode* parseForLoop(Parser* parser) {
    printf("DEBUG: Entering parseForLoop() at line %d\n", parser->currentToken.line);
 
    if (parser->currentToken.type != TOKEN_FOR) {
        printf("Error: expected 'for' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume 'for'
     
    if (parser->currentToken.type != TOKEN_LPAREN) {   
        printf("Error: expected '(' after 'for' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume '('
            
    // Parse for-loop components using helper methods
    ASTNode* init = parseForLoopInitializer(parser);
    ASTNode* condition = parseForLoopCondition(parser);
    ASTNode* increment = parseForLoopIncrement(parser);
            
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printf("Error: expected ')' after for-loop increment at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume ')'
    
    ASTNode* body = NULL;
        
    if (parser->currentToken.type == TOKEN_LBRACE) {
        body = parseBlock(parser);  // true `{ ... }` block
    } else {
        // Single statement — wrap it in a block manually
        ASTNode* single = parseStatement(parser);
        if (!single) {
            printf("Error: Invalid statement in 'for' loop at line %d\n", parser->currentToken.line);
            return NULL;
        }
        ASTNode** stmts = malloc(sizeof(ASTNode*));
        stmts[0] = single;
        body = createBlockNode(stmts, 1);
    }
            
    return createForLoopNode(init, condition, increment, body);
}


ASTNode* parseForLoopInitializer(Parser* parser) {
    printf("DEBUG: Parsing for-loop initializer at line %d\n", parser->currentToken.line);
    ASTNode* init = NULL;
    
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        if (parser->currentToken.type == TOKEN_INT || parser->currentToken.type == TOKEN_FLOAT ||
            parser->currentToken.type == TOKEN_DOUBLE || parser->currentToken.type == TOKEN_LONG ||
            parser->currentToken.type == TOKEN_SHORT || parser->currentToken.type == TOKEN_BOOL || 
            parser->currentToken.type == TOKEN_SIGNED || parser->currentToken.type == TOKEN_UNSIGNED ||
            parser->currentToken.type == TOKEN_CONST) {
            init = parseDeclarationForLoop(parser);
        } else {
            init = parseAssignmentExpression(parser);  //  Use assignment-only for single expression
        }
         
        if (!init) {
            printf("Error: Invalid initializer in for-loop at line %d\n", parser->currentToken.line);
            return NULL;
        }
    }
     
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printf("Error: expected ';' after for-loop initializer at line %d\n", 
				parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume ';'
    return init;
}

ASTNode* parseForLoopCondition(Parser* parser) {
    printf("DEBUG: Parsing for-loop condition at line %d\n", parser->currentToken.line);
    ASTNode* condition = NULL;
    
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        condition = parseAssignmentExpression(parser);  // Avoid comma expressions here
        if (!condition) {
            printf("Error: Invalid condition in for-loop at line %d\n", parser->currentToken.line);
            return NULL;
        }
    }
     
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printf("Error: expected ';' after for-loop condition at line %d\n", 
			parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume ';'
    return condition;
}
 
 
ASTNode* parseForLoopIncrement(Parser* parser) {
    printf("DEBUG: Parsing for-loop increment at line %d\n", parser->currentToken.line);
    
    ASTNode* increment = NULL;
    
    //  Allow prefix or postfix forms and compound expressions
    if (parser->currentToken.type != TOKEN_RPAREN) {
        increment = parseCommaExpression(parser);  // Full expression parsing
        if (!increment) {
            printf("Error: Invalid increment in for-loop at line %d\n", parser->currentToken.line);
            return NULL;
        }
    }
     
    return increment;
}





// jump locations
ASTNode* parseReturnStatement(Parser* parser) {
    if (parser->currentToken.type != TOKEN_RETURN) {
        printf("Error: expected 'return' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume 'return'

    ASTNode* expr = NULL;

    // If not a semicolon, parse a return expression
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        if (parser->mode == PARSER_MODE_PRATT) {
            expr = parseExpressionPratt(parser, -1);  // Loosest floor at statement scope
            printf("DEBUG: Parsed return expr (PRATT, rbp=-1)\n");
        } else {
            expr = parseExpression(parser);
            printf("DEBUG: Parsed return expr (RECURSIVE)\n");
        }

        if (!expr) {
            printf("Error: Invalid return expression at line %d\n", parser->currentToken.line);
            return NULL;
        }
    }

    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printf("Error: expected ';' after return statement at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume ';'

    // If it's a comma expression, emit multiple returns as a block (unchanged)
    if (expr && expr->type == AST_COMMA_EXPRESSION) {
        ASTNode* block = malloc(sizeof(ASTNode));
        block->type = AST_BLOCK;
        block->block.statementCount = expr->commaExpr.exprCount;
        block->block.statements = malloc(sizeof(ASTNode*) * expr->commaExpr.exprCount);
        block->nextStmt = NULL;

        for (size_t i = 0; i < expr->commaExpr.exprCount; i++) {
            block->block.statements[i] = createReturnNode(expr->commaExpr.expressions[i]);
        }
        return block;
    }

    return createReturnNode(expr);
}


ASTNode* parseBreakStatement(Parser* parser) {
    if (parser->currentToken.type != TOKEN_BREAK) {
        printf("Error: expected 'break' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume 'break'
    
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printf("Error: expected ';' after 'break' statement at line %d\n", 
			parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume ';'
    return createBreakNode();
}
        
ASTNode* parseContinueStatement(Parser* parser) {
    if (parser->currentToken.type != TOKEN_CONTINUE) {
        printf("Error: expected 'continue' at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume 'continue'
            
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printf("Error: expected ';' after 'continue' statement at line %d\n",
		 parser->currentToken.line);
        return NULL;
    }
    advance(parser); // Consume ';'
    return createContinueNode();
}

ASTNode* parseGotoStatement(Parser* parser) {
    if (parser->currentToken.type != TOKEN_GOTO) {
        printParseError("Expected 'goto'", parser);
        return NULL;
    }
    advance(parser); // consume 'goto'
    
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected label name after 'goto'", parser);
        return NULL;
    }
     
    char* label = parser->currentToken.value;
    advance(parser); // consume label
    
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printParseError("Expected ';' after label in goto statement", parser);
        return NULL;
    }
    advance(parser); // consume ';'
    
    return createGotoStatementNode(label);
}

ASTNode* parseLabel(Parser* parser) {
    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        Token next = peekNextToken(parser);
        if (next.type == TOKEN_COLON) {
            const char* labelName = parser->currentToken.value;
            
            advance(parser);  // consume identifier
            advance(parser);  // consume colon
            
            ASTNode* statement = parseStatement(parser);
            return createLabelDeclarationNode(labelName, statement);
        }
    }
    return NULL;
}



ASTNode* parseAsmStatement(Parser* parser) {
    advance(parser); // consume 'asm'
    
    if (parser->currentToken.type != TOKEN_STRING) {
        printParseError("Expected string literal after 'asm'", parser);
        return NULL;
    }
     
    char* asmText = strdup(parser->currentToken.value);  // Copy string content
    advance(parser);  // consume string
    
    if (parser->currentToken.type == TOKEN_SEMICOLON) {
        advance(parser);  // consume semicolon
    }
     
    return createAsmNode(asmText);
}
