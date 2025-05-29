#include "parser_func.h"
#include "parser_helpers.h"
#include "parser_main.h"
#include "parser_decl.h"
#include "parser_decl.h"
#include "parser_expr.h"



ASTNode* parseFunctionDefinition(Parser* parser, ParsedType returnType) {
    // Ensure function name is present
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printf("Error: expected function name at line %d\n", parser->currentToken.line);
        return NULL;
    }

    ASTNode* funcName = createIdentifierNode(parser->currentToken.value);
    advance(parser);  // Consume function name

    // Expect opening parenthesis
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printf("Error: expected '(' after function name at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser);  // Consume '('

    // Parse parameter list (optional)
    ASTNode** paramList = NULL;
    size_t paramCount = 0;

    if (parser->currentToken.type != TOKEN_RPAREN) {
        paramList = parseParameterList(parser, &paramCount);  // Can be stubbed
    }

    // Confirm closing parenthesis
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printf("Error: expected ')' after parameter list at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser);  // Consume ')'

    // Determine next step: declaration or definition
    if (parser->currentToken.type == TOKEN_SEMICOLON) {
        advance(parser);  // Consume ';'
        return createFunctionDeclarationNode(returnType, funcName->valueNode.value);  // Declaration path
    }

    if (parser->currentToken.type == TOKEN_LBRACE) {
        ASTNode* body = parseBlock(parser);  // Definition path
        if (!body) {
            printf("Error: Failed to parse function body at line %d\n", parser->currentToken.line);
            return NULL;
        }
        return createFunctionDefinitionNode(returnType, funcName, paramList, body, paramCount);
    }

    // If neither, report error
    printParseError("Expected ';' or '{' after function declaration", parser);
    return NULL;
}


ASTNode** parseParameterList(Parser* parser, size_t* paramCount) {
    printf("DEBUG: Entering parseParameterList() at line %d\n", parser->currentToken.line);
            
    *paramCount = 0;
    size_t paramCapacity = 4;
    ASTNode** paramList = malloc(paramCapacity * sizeof(ASTNode*));
    size_t unnamedIndex = 0;
    
    while (parser->currentToken.type != TOKEN_RPAREN && parser->currentToken.type != TOKEN_EOF) {
        ParsedType paramType = parseType(parser);
        
        ASTNode* paramName = NULL;
        if (parser->currentToken.type == TOKEN_IDENTIFIER) {
            paramName = createIdentifierNode(parser->currentToken.value);
            advance(parser);  // consume identifier
        } else {
            // Create synthetic name for unnamed parameter
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "__unnamed_param%zu", unnamedIndex++);
            paramName = createIdentifierNode(strdup(buffer));
        }
        
        ASTNode** paramNames = malloc(sizeof(ASTNode*));
        DesignatedInit** initializers = malloc(sizeof(ASTNode*));
        paramNames[0] = paramName;
        initializers[0] = NULL;
    
        ASTNode* paramDecl = createVariableDeclarationNode(paramType, paramNames, initializers, 1);
    
        // Expand if needed
        if (*paramCount >= paramCapacity) {
            paramCapacity *= 2;
            paramList = realloc(paramList, paramCapacity * sizeof(ASTNode*));
        }
        
        paramList[*paramCount] = paramDecl;
        (*paramCount)++;
    
        // Handle ',' or error
        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser);
        } else if (parser->currentToken.type != TOKEN_RPAREN) {
            printParseError("Expected ',' or ')' in parameter list", parser);
            free(paramList);  
            return NULL;
        }
    }   
            
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' to close parameter list", parser);
        free(paramList);
        return NULL;
    }
                    
    // advance(parser);  // consume ')'
    printf("DEBUG: Successfully parsed %zu function parameters\n", *paramCount);
    return paramList;
}

ASTNode* parseFunctionCall(Parser* parser, ASTNode* callee) {
    printf("DEBUG: Entering parseFunctionCall() at line %d\n", parser->currentToken.line);
                
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printParseError("Expected '(' to start function call", parser);
        return NULL;
    }
    advance(parser); // Consume '('
                
    // Prepare dynamic argument list
    size_t argCapacity = 4;
    size_t argCount = 0;
    ASTNode** argList = malloc(argCapacity * sizeof(ASTNode*));
    if (!argList) {
        printf("Error: Memory allocation failed for function arguments\n");
        return NULL;
    }
        
    // Parse arguments if not an empty call
    if (parser->currentToken.type != TOKEN_RPAREN) {
        while (1) {
            ASTNode* arg = parseExpression(parser);
            if (!arg) {
                printf("Error: Invalid function argument at line %d\n", parser->currentToken.line);
                free(argList);
                return NULL;
            }

            // Expand if needed
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
                advance(parser); // Consume ','
                // Allow trailing comma, but check next token
                if (parser->currentToken.type == TOKEN_RPAREN) {
                    printf("Warning: Trailing comma in argument list at line %d\n",
                                parser->currentToken.line);
                    break;
                }   
            } else {
                break;
            }
        }
    }
        
    // Expect closing ')'
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' to close function call", parser);
        free(argList);
        return NULL;
    }
    
    advance(parser); // Consume ')'
        
    printf("DEBUG: Function call parsed with %zu argument(s)\n", argCount);
    return createFunctionCallNode(callee, argList, argCount);
} 



ASTNode* parseFunctionPointerDeclaration(Parser* parser) {
    printf("DEBUG: Entering parseFunctionPointerDeclarator() at line %d\n", 
			parser->currentToken.line);
     
    // Parse the return type first (modifiers + primitive/user-defined type + pointer depth)
    ParsedType returnType = parseType(parser);
    if (returnType.kind == TYPE_INVALID) {
        printParseError("Invalid return type in function pointer declarator", parser);
        return NULL;
    }
        
    // Expect '('
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printParseError("Expected '(' to start function pointer declarator", parser);
        return NULL;
    }
    advance(parser);  // consume '('
        
    // Expect '*'
    if (parser->currentToken.type != TOKEN_ASTERISK) {
        printParseError("Expected '*' in function pointer declarator", parser);
        return NULL;
    }
    advance(parser);  // consume '*'
     
    // Expect identifier (name of function pointer)
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected function pointer name", parser);
        return NULL;
    }
    ASTNode* name = createIdentifierNode(parser->currentToken.value);
    advance(parser);  // consume name
                 
    // Expect ')'
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' after function pointer name", parser);
        return NULL;
    }       
    advance(parser);  // consume ')'

    // Expect '(' to start parameter list
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printParseError("Expected '(' to start parameter list in function pointer declarator", 
				parser);
        return NULL;
    }
    advance(parser);  // consume '('
     
    // Parse parameters
    ASTNode** paramList = NULL;
    size_t paramCount = 0;
    if (parser->currentToken.type != TOKEN_RPAREN) {
        paramList = parseParameterList(parser, &paramCount);
        if (!paramList) {
            printf("Error: Invalid parameters in function pointer declaration at line %d\n",
                   parser->currentToken.line);
            return NULL;
        }
    }
        
    if (parser->currentToken.type == TOKEN_RPAREN) {
            advance(parser);  // consume ')'
    }

    if (parser->currentToken.type == TOKEN_SEMICOLON) {
            advance(parser);  // consume ';'
    }
    // Create the AST node
    return createFunctionPointerDeclarationNode(returnType, name, paramList, paramCount);
}
