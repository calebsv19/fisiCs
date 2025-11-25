#include "parser_func.h"
#include "Parser/Helpers/parser_helpers.h"
#include "parser_main.h"
#include "parser_decl.h"
#include "parser_decl.h"
#include "Parser/Expr/parser_expr.h"
#include "Parser/Expr/parser_expr_pratt.h"
#include "Parser/Helpers/parser_attributes.h"



ASTNode* parseFunctionDefinition(Parser* parser, ParsedType returnType) {
    ParsedType funcReturnType = parsedTypeClone(&returnType);
    int pointerDepth = parsePointerChain(parser);
    applyPointerChainToType(&funcReturnType, pointerDepth);

    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printf("Error: expected function name at line %d\n", parser->currentToken.line);
        return NULL;
    }

    ASTNode* funcName = createIdentifierNode(parser->currentToken.value);
    if (funcName) funcName->line = parser->currentToken.line;
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

        ParsedType paramType = parseType(parser);
        int pointerDepth = parsePointerChain(parser);
        applyPointerChainToType(&paramType, pointerDepth);
        
        ASTNode* paramName = NULL;
        if (parser->currentToken.type == TOKEN_IDENTIFIER) {
            paramName = createIdentifierNode(parser->currentToken.value);
            if (paramName) paramName->line = parser->currentToken.line;
            advance(parser);  // consume identifier
        } else {
            // Create synthetic name for unnamed parameter
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "__unnamed_param%zu", unnamedIndex++);
            paramName = createIdentifierNode(strdup(buffer));
        }
        
        ASTNode** paramNames = malloc(sizeof(ASTNode*));
        DesignatedInit** initializers = malloc(sizeof(DesignatedInit*));
        paramNames[0] = paramName;
        initializers[0] = NULL;
    
        ASTNode* paramDecl = createVariableDeclarationNode(paramType, paramNames, initializers, 1);
        if (paramDecl) {
            ParsedType* per = malloc(sizeof(ParsedType));
            if (per) {
                per[0] = paramType;
                paramDecl->varDecl.declaredTypes = per;
            }
            astNodeCloneTypeAttributes(paramDecl, &paramType);
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



ASTNode* parseFunctionPointerDeclaration(Parser* parser, ParsedType returnType) {
    PARSER_DEBUG_PRINTF("DEBUG: Entering parseFunctionPointerDeclaration() at line %d\n",
           parser->currentToken.line);

    /* 2) Expect '(' then one-or-more '*' then an identifier then ')' */
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printParseError("Expected '(' after return type", parser);
        return NULL;
    }
    advance(parser); /* consume '(' */

    int innerStars = 0;
    if (parser->currentToken.type != TOKEN_ASTERISK) {
        printParseError("Expected '*' in function pointer declarator", parser);
        return NULL;
    }
    do {
        ++innerStars;
        advance(parser);
        /* Optional: qualifiers after each '*' */
        /* while (isTypeQualifier(parser->currentToken.type)) advance(parser); */
    } while (parser->currentToken.type == TOKEN_ASTERISK);

    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected identifier for function pointer name", parser);
        return NULL;
    }
    ASTNode* name = createIdentifierNode(parser->currentToken.value);
    if (name) name->line = parser->currentToken.line;
    advance(parser); /* consume name */

    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' after function pointer name", parser);
        return NULL;
    }
    advance(parser); /* consume ')' */

    /* 3) Parameter list: '(' param-type-list ')'  (support 'void' or comma-separated types) */
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printParseError("Expected '(' to start parameter list", parser);
        return NULL;
    }
    advance(parser); /* consume '(' */

    /* Gather parameter *types* for the ParsedType; we can still build the AST param list
       later if your node wants names.
     */
    size_t cap = 4, cnt = 0;
    ParsedType* fpParams = (ParsedType*)malloc(cap * sizeof(ParsedType));
    if (!fpParams) cap = 0;

    bool onlyVoid = false;
    if (parser->currentToken.type != TOKEN_RPAREN) {
        while (1) {
            if (parser->currentToken.type == TOKEN_VOID) {
                advance(parser);
                if (parser->currentToken.type == TOKEN_RPAREN) {
                    onlyVoid = true;
                }
                if (onlyVoid) break;
            }

            ParsedType p = parseType(parser);
            int paramPtrDepth = parsePointerChain(parser);
            applyPointerChainToType(&p, paramPtrDepth);
            if (p.kind == TYPE_INVALID) {
                printParseError("Invalid parameter type in function pointer declaration", parser);
                if (fpParams) free(fpParams);
                return NULL;
            }

            /* Optional parameter name (ignored) */
            if (parser->currentToken.type == TOKEN_IDENTIFIER) {
                TokenType look = peekNextToken(parser).type;
                if (look == TOKEN_COMMA || look == TOKEN_RPAREN) {
                    advance(parser);
                }
            }

            if (!onlyVoid) {
                if (cnt == cap) {
                    size_t ncap = cap ? cap * 2 : 4;
                    ParsedType* nbuf = (ParsedType*)realloc(fpParams, ncap * sizeof(ParsedType));
                    if (nbuf) { fpParams = nbuf; cap = ncap; }
                }
                if (cnt < cap) fpParams[cnt++] = p;
            }

            if (parser->currentToken.type == TOKEN_COMMA) {
                advance(parser);
                continue;
            }
            break;
        }
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' to close parameter list", parser);
        if (fpParams) free(fpParams);
        return NULL;
    }
    advance(parser); /* consume ')' */

    /* 4) Build the full function-pointer type: base return type + innerStars + params */
    if (onlyVoid) {
        parsedTypeSetFunctionPointer(&returnType, 0, NULL);
    } else {
        parsedTypeSetFunctionPointer(&returnType, cnt, fpParams);
    }
    if (fpParams) free(fpParams);

    /* The stars inside '(*name)' increase the pointer depth of the function pointer */
    parsedTypeAddPointerDepth(&returnType, innerStars);

    /* 5) Optional initializer: = <expression> ;  (use Pratt) */
    ASTNode* initializer = NULL;
    if (parser->currentToken.type == TOKEN_ASSIGN) {
        advance(parser); /* consume '=' */
        initializer = parseExpressionPratt(parser, 0);
        if (!initializer) {
            printParseError("Invalid initializer for function pointer", parser);
            return NULL;
        }
    }
    size_t fpAttrCount = 0;
    ASTAttribute** fpAttrs = parserParseAttributeSpecifiers(parser, &fpAttrCount);

    /* 6) End of declaration ;  (TODO: support comma-list of FP declarators if you want) */
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printParseError("Expected ';' after function pointer declaration", parser);
        astAttributeListDestroy(fpAttrs, fpAttrCount);
        free(fpAttrs);
        return NULL;
    }
    advance(parser); /* consume ';' */

    /* Reuse your existing node factory; if it needs params as AST nodes, you can extend it later.
       For now we’ve fully encoded the signature in 'returnType' (now a function-pointer type),
       and 'initializer' is the Pratt-parsed RHS if present.
    */
    ASTNode* node = createFunctionPointerDeclarationNode(returnType, name, /*paramList*/NULL, /*paramCount*/0, initializer);
    if (node) {
        astNodeCloneTypeAttributes(node, &returnType);
        astNodeAppendAttributes(node, fpAttrs, fpAttrCount);
        fpAttrs = NULL;
        fpAttrCount = 0;
    } else {
        astAttributeListDestroy(fpAttrs, fpAttrCount);
        free(fpAttrs);
    }
    return node;
}
