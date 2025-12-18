#include "parser_func.h"
#include "Parser/Helpers/parser_helpers.h"
#include "parser_main.h"
#include "parser_decl.h"
#include "Parser/Expr/parser_expr.h"
#include "Parser/Expr/parser_expr_pratt.h"
#include "Parser/Helpers/parser_attributes.h"



ASTNode* parseFunctionDefinition(Parser* parser, ParsedType returnType) {
    ParsedType funcReturnType = parsedTypeClone(&returnType);
    PointerChain chain = parsePointerChain(parser);
    applyPointerChainToType(&funcReturnType, &chain);
    pointerChainFree(&chain);

    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printf("Error: expected function name at line %d\n", parser->currentToken.line);
        return NULL;
    }

    ASTNode* funcName = createIdentifierNode(parser->currentToken.value);
    astNodeSetProvenance(funcName, &parser->currentToken);
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
    bool isVariadic = false;

    if (parser->currentToken.type != TOKEN_RPAREN) {
        paramList = parseParameterList(parser, &paramCount, &isVariadic);
        if (!paramList) {
            return NULL;
        }
    }

    // Confirm closing parenthesis
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printf("Error: expected ')' after parameter list at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser);  // Consume ')'
    size_t funcAttrCount = 0;
    ASTAttribute** funcAttrs = parserParseAttributeSpecifiers(parser, &funcAttrCount);

    // Determine next step: declaration or definition
    if (parser->currentToken.type == TOKEN_SEMICOLON) {
        advance(parser);  // Consume ';'
        ASTNode* decl = createFunctionDeclarationNode(funcReturnType, funcName, paramList, paramCount, isVariadic);
        if (decl && decl->functionDecl.funcName) {
            decl->functionDecl.funcName->line = funcName->line;
            astNodeCloneTypeAttributes(decl, &funcReturnType);
            astNodeAppendAttributes(decl, funcAttrs, funcAttrCount);
            funcAttrs = NULL;
            funcAttrCount = 0;
        }
        return decl;
    }

    if (parser->currentToken.type == TOKEN_LBRACE) {
        ASTNode* body = parseBlock(parser);  // Definition path
        if (!body) {
            printf("Error: Failed to parse function body at line %d\n", parser->currentToken.line);
            return NULL;
        }
        ASTNode* def = createFunctionDefinitionNode(funcReturnType, funcName, paramList, body, paramCount, isVariadic);
        if (def) {
            astNodeCloneTypeAttributes(def, &funcReturnType);
            astNodeAppendAttributes(def, funcAttrs, funcAttrCount);
            funcAttrs = NULL;
            funcAttrCount = 0;
        }
        return def;
    }

    // If neither, report error
    printParseError("Expected ';' or '{' after function declaration", parser);
    astAttributeListDestroy(funcAttrs, funcAttrCount);
    free(funcAttrs);
    return NULL;
}


ASTNode** parseParameterList(Parser* parser, size_t* paramCount, bool* isVariadic) {
    PARSER_DEBUG_PRINTF("DEBUG: Entering parseParameterList() at line %d\n", parser->currentToken.line);
            
    *paramCount = 0;
    if (isVariadic) {
        *isVariadic = false;
    }
    size_t paramCapacity = 4;
    ASTNode** paramList = malloc(paramCapacity * sizeof(ASTNode*));
    size_t unnamedIndex = 0;
    
    while (parser->currentToken.type != TOKEN_RPAREN && parser->currentToken.type != TOKEN_EOF) {
        if (parser->currentToken.type == TOKEN_ELLIPSIS) {
            if (isVariadic) {
                *isVariadic = true;
            }
            advance(parser);
            break;
        }

        size_t leadingAttrCount = 0;
        ASTAttribute** leadingAttrs = parserParseAttributeSpecifiers(parser, &leadingAttrCount);
        ParsedType paramBase = parseType(parser);
        parsedTypeAdoptAttributes(&paramBase, leadingAttrs, leadingAttrCount);
        ParsedDeclarator decl;
        if (!parserParseDeclarator(parser,
                                   &paramBase,
                                   false,
                                   false,
                                   &decl)) {
            printParseError("Invalid parameter declarator", parser);
            free(paramList);
            return NULL;
        }
        parsedTypeFree(&paramBase);

        size_t trailingAttrCount = 0;
        ASTAttribute** trailingAttrs = parserParseAttributeSpecifiers(parser, &trailingAttrCount);
        parsedTypeAdoptAttributes(&decl.type, trailingAttrs, trailingAttrCount);

        ASTNode* paramName = decl.identifier;
        if (!paramName) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "__unnamed_param%zu", unnamedIndex++);
            paramName = createIdentifierNode(strdup(buffer));
        }
        
        ASTNode** paramNames = malloc(sizeof(ASTNode*));
        DesignatedInit** initializers = malloc(sizeof(DesignatedInit*));
        paramNames[0] = paramName;
        initializers[0] = NULL;
    
        ASTNode* paramDecl = createVariableDeclarationNode(decl.type, paramNames, initializers, 1);
        if (paramDecl) {
            ParsedType* per = malloc(sizeof(ParsedType));
            if (per) {
                per[0] = decl.type;
                paramDecl->varDecl.declaredTypes = per;
            }
            astNodeCloneTypeAttributes(paramDecl, &decl.type);
        }
    
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
        } else if (parser->currentToken.type != TOKEN_RPAREN && parser->currentToken.type != TOKEN_ELLIPSIS) {
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
    if (isVariadic && *isVariadic && *paramCount == 0) {
        printParseError("Variadic parameter list requires at least one named parameter", parser);
        free(paramList);
        return NULL;
    }
                    
    // advance(parser);  // consume ')'
    PARSER_DEBUG_PRINTF("DEBUG: Successfully parsed %zu function parameters\n", *paramCount);
    return paramList;
}

ASTNode* parseFunctionCall(Parser* parser, ASTNode* callee) {
    PARSER_DEBUG_PRINTF("DEBUG: Entering parseFunctionCall() at line %d\n", parser->currentToken.line);
                
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
        
    PARSER_DEBUG_PRINTF("DEBUG: Function call parsed with %zu argument(s)\n", argCount);
    return createFunctionCallNode(callee, argList, argCount);
} 
