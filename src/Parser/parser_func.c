// SPDX-License-Identifier: Apache-2.0

#include "parser_func.h"
#include "Parser/Helpers/parser_helpers.h"
#include "parser_main.h"
#include "parser_decl.h"
#include "Parser/Expr/parser_expr.h"
#include "Parser/Expr/parser_expr_pratt.h"
#include "Parser/Helpers/parser_attributes.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    ASTNode* nameNode;
    ParsedType type;
    bool declared;
} KNRParam;

static ParsedType makeImplicitIntType(void) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_PRIMITIVE;
    t.primitiveType = TOKEN_INT;
    t.isSigned = true;
    return t;
}

static int findKNRParam(KNRParam* params, size_t count, const char* name) {
    if (!params || !name) return -1;
    for (size_t i = 0; i < count; ++i) {
        if (params[i].nameNode &&
            params[i].nameNode->type == AST_IDENTIFIER &&
            params[i].nameNode->valueNode.value &&
            strcmp(params[i].nameNode->valueNode.value, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static bool parseKNRIdentifierList(Parser* parser, KNRParam** outParams, size_t* outCount) {
    if (!parser || !outParams || !outCount) return false;
    *outParams = NULL;
    *outCount = 0;

    size_t capacity = 0;
    while (parser->currentToken.type != TOKEN_RPAREN &&
           parser->currentToken.type != TOKEN_EOF) {
        if (parser->currentToken.type != TOKEN_IDENTIFIER) {
            printParseError("Expected identifier in old-style parameter list", parser);
            return false;
        }
        if (*outCount == capacity) {
            size_t newCap = capacity == 0 ? 4 : capacity * 2;
            KNRParam* grown = realloc(*outParams, newCap * sizeof(KNRParam));
            if (!grown) {
                printParseError("Out of memory parsing parameters", parser);
                free(*outParams);
                *outParams = NULL;
                *outCount = 0;
                return false;
            }
            *outParams = grown;
            capacity = newCap;
        }
        (*outParams)[*outCount].nameNode = createIdentifierNode(parser->currentToken.value);
        astNodeSetProvenance((*outParams)[*outCount].nameNode, &parser->currentToken);
        (*outParams)[*outCount].type = makeImplicitIntType();
        (*outParams)[*outCount].declared = false;
        ++(*outCount);
        advance(parser);
        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser);
            continue;
        }
        break;
    }

    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("Expected ')' to close parameter list", parser);
        free(*outParams);
        *outParams = NULL;
        *outCount = 0;
        return false;
    }
    return true;
}

static bool parseOldStyleParamDeclarations(Parser* parser, KNRParam* params, size_t paramCount) {
    if (!parser || !params) return false;
    while (parser->currentToken.type != TOKEN_LBRACE &&
           parser->currentToken.type != TOKEN_EOF) {
        ParsedType base = parseType(parser);
        if (base.kind == TYPE_INVALID) {
            parsedTypeFree(&base);
            return false;
        }

        bool sawAny = false;
        while (1) {
            ParsedDeclarator decl;
            if (!parserParseDeclarator(parser, &base, false, true, false, &decl)) {
                parsedTypeFree(&base);
                printParseError("Invalid parameter declarator", parser);
                return false;
            }
            sawAny = true;
            if (!decl.identifier || !decl.identifier->valueNode.value) {
                parserDeclaratorDestroy(&decl);
                parsedTypeFree(&base);
                printParseError("Parameter name required", parser);
                return false;
            }
            int idx = findKNRParam(params, paramCount, decl.identifier->valueNode.value);
            if (idx < 0) {
                parserDeclaratorDestroy(&decl);
                parsedTypeFree(&base);
                printParseError("Declaration does not match an old-style parameter", parser);
                return false;
            }
            if (params[idx].declared) {
                parserDeclaratorDestroy(&decl);
                parsedTypeFree(&base);
                printParseError("Duplicate declaration for parameter", parser);
                return false;
            }
            params[idx].type = decl.type;
            params[idx].declared = true;
            params[idx].nameNode = decl.identifier;
            decl.type.kind = TYPE_INVALID;
            decl.identifier = NULL;
            parserDeclaratorDestroy(&decl);

            if (parser->currentToken.type == TOKEN_COMMA) {
                advance(parser);
                continue;
            }
            break;
        }
        parsedTypeFree(&base);
        if (!sawAny) {
            printParseError("Expected parameter declarator", parser);
            return false;
        }
        if (parser->currentToken.type != TOKEN_SEMICOLON) {
            printParseError("Expected ';' after parameter declaration", parser);
            return false;
        }
        advance(parser); // consume ';'
    }
    return true;
}

static ASTNode** buildKNRParamDecls(KNRParam* params, size_t paramCount) {
    if (!params) return NULL;
    ASTNode** list = calloc(paramCount ? paramCount : 1, sizeof(ASTNode*));
    if (!list) return NULL;
    for (size_t i = 0; i < paramCount; ++i) {
        DesignatedInit** inits = calloc(1, sizeof(DesignatedInit*));
        ASTNode** names = malloc(sizeof(ASTNode*));
        if (!inits || !names) {
            free(inits);
            free(names);
            for (size_t j = 0; j < i; ++j) {
                // leave existing nodes allocated; caller can free separately if needed
            }
            free(list);
            return NULL;
        }
        names[0] = params[i].nameNode;
        if (!params[i].declared) {
            params[i].type = makeImplicitIntType();
        }
        ASTNode* paramDecl = createVariableDeclarationNode(params[i].type, names, inits, 1);
        if (paramDecl) {
            ParsedType* per = malloc(sizeof(ParsedType));
            if (per) {
                *per = params[i].type;
                paramDecl->varDecl.declaredTypes = per;
            }
            astNodeCloneTypeAttributes(paramDecl, &params[i].type);
        }
        list[i] = paramDecl;
    }
    return list;
}

static bool currentListIsKNR(Parser* parser) {
    Parser probe = cloneParserWithFreshLexer(parser);
    bool ok = true;
    bool sawIdent = false;
    while (probe.currentToken.type != TOKEN_RPAREN &&
           probe.currentToken.type != TOKEN_EOF) {
        if (probe.currentToken.type != TOKEN_IDENTIFIER) {
            ok = false;
            break;
        }
        sawIdent = true;
        advance(&probe);
        if (probe.currentToken.type == TOKEN_COMMA) {
            advance(&probe);
            continue;
        }
        if (probe.currentToken.type != TOKEN_RPAREN) {
            ok = false;
            break;
        }
    }
    ok = ok && sawIdent && probe.currentToken.type == TOKEN_RPAREN;
    if (ok) {
        Token after = peekNextToken(&probe);
        if (after.type == TOKEN_SEMICOLON) {
            ok = false;
        }
    }
    freeParserClone(&probe);
    return ok;
}

static bool currentTokenIsTransparentGroupedIdentifier(Parser* parser) {
    if (!parser || parser->currentToken.type != TOKEN_LPAREN) {
        return false;
    }
    Parser probe = cloneParserWithFreshLexer(parser);
    advance(&probe); // consume '('
    bool transparent = false;
    if (probe.currentToken.type == TOKEN_IDENTIFIER) {
        advance(&probe);
        transparent = (probe.currentToken.type == TOKEN_RPAREN);
    }
    freeParserClone(&probe);
    return transparent;
}



ASTNode* parseFunctionDefinition(Parser* parser, ParsedType returnType) {
    if (parser->currentToken.type == TOKEN_LPAREN) {
        ParsedDeclarator decl;
        if (!parserParseDeclarator(parser, &returnType, true, true, true, &decl)) {
            printParseError("Invalid function declarator", parser);
            return NULL;
        }
        if (!decl.declaresFunction || !decl.identifier) {
            printParseError("Expected function declarator", parser);
            parsedTypeFree(&decl.type);
            return NULL;
        }
        ParsedType funcReturnType = parsedTypeDeclaredFunctionReturnType(&decl.type);
        ASTNode* funcName = decl.identifier;
        ASTNode** paramList = decl.functionParameters;
        size_t paramCount = decl.functionParamCount;
        bool isVariadic = decl.functionIsVariadic;
        size_t funcAttrCount = 0;
        ASTAttribute** funcAttrs = parserParseAttributeSpecifiers(parser, &funcAttrCount);

        if (parser->currentToken.type == TOKEN_SEMICOLON) {
            advance(parser);
            ASTNode* declNode = createFunctionDeclarationNode(funcReturnType,
                                                              funcName,
                                                              paramList,
                                                              paramCount,
                                                              isVariadic);
            if (declNode) {
                astNodeCloneTypeAttributes(declNode, &funcReturnType);
                astNodeAppendAttributes(declNode, funcAttrs, funcAttrCount);
                funcAttrs = NULL;
                funcAttrCount = 0;
            }
            parsedTypeFree(&decl.type);
            astAttributeListDestroy(funcAttrs, funcAttrCount);
            free(funcAttrs);
            return declNode;
        }

        if (parser->currentToken.type == TOKEN_LBRACE) {
            ASTNode* body = parseBlock(parser);
            if (!body) {
                printf("Error: Failed to parse function body at line %d\n", parser->currentToken.line);
                parsedTypeFree(&decl.type);
                astAttributeListDestroy(funcAttrs, funcAttrCount);
                free(funcAttrs);
                return NULL;
            }
            ASTNode* def = createFunctionDefinitionNode(funcReturnType,
                                                        funcName,
                                                        paramList,
                                                        body,
                                                        paramCount,
                                                        isVariadic);
            if (def) {
                astNodeCloneTypeAttributes(def, &funcReturnType);
                astNodeAppendAttributes(def, funcAttrs, funcAttrCount);
                funcAttrs = NULL;
                funcAttrCount = 0;
            }
            parsedTypeFree(&decl.type);
            astAttributeListDestroy(funcAttrs, funcAttrCount);
            free(funcAttrs);
            return def;
        }

        printParseError("Expected ';' or '{' after function declaration", parser);
        parsedTypeFree(&decl.type);
        astAttributeListDestroy(funcAttrs, funcAttrCount);
        free(funcAttrs);
        return NULL;
    }

    ParsedType funcReturnType = parsedTypeClone(&returnType);
    PointerChain chain = parsePointerChain(parser);
    applyPointerChainToType(&funcReturnType, &chain);
    pointerChainFree(&chain);

    ASTNode* funcName = NULL;
    if (parser->currentToken.type == TOKEN_LPAREN &&
        currentTokenIsTransparentGroupedIdentifier(parser)) {
        advance(parser); // consume '('
        funcName = createIdentifierNode(parser->currentToken.value);
        astNodeSetProvenance(funcName, &parser->currentToken);
        advance(parser); // consume identifier
        advance(parser); // consume ')'
    } else if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        /* Gracefully consume complex declarators like
         *   void (*bsd_signal(int, void (*)(int)))(int);
         * so we stay in sync instead of erroring. */
        ParsedDeclarator fallbackDecl;
        ParsedType base = parsedTypeClone(&returnType);
        Parser probe = cloneParserWithFreshLexer(parser);
        bool ok = parserParseDeclarator(&probe, &base, true, true, false, &fallbackDecl);
        if (ok) {
            /* Advance the real parser to where the successful probe ended. */
            while (parser->cursor < probe.cursor) {
                advance(parser);
            }
            parserDeclaratorDestroy(&fallbackDecl);
        } else {
            /* Skip tokens conservatively to next ';' or '{'. */
            while (parser->currentToken.type != TOKEN_SEMICOLON &&
                   parser->currentToken.type != TOKEN_LBRACE &&
                   parser->currentToken.type != TOKEN_EOF) {
                advance(parser);
            }
            if (parser->currentToken.type == TOKEN_SEMICOLON ||
                parser->currentToken.type == TOKEN_LBRACE) {
                advance(parser);
            }
        }
        parsedTypeFree(&base);
        freeParserClone(&probe);
        return NULL;
    } else {
        funcName = createIdentifierNode(parser->currentToken.value);
        astNodeSetProvenance(funcName, &parser->currentToken);
        advance(parser);  // Consume function name
    }

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
    bool isKNR = false;
    KNRParam* knrParams = NULL;
    size_t knrCount = 0;

    if (parser->currentToken.type != TOKEN_RPAREN) {
        if (currentListIsKNR(parser)) {
            isKNR = true;
            if (!parseKNRIdentifierList(parser, &knrParams, &knrCount)) {
                free(knrParams);
                return NULL;
            }
        } else {
            paramList = parseParameterList(parser, &paramCount, &isVariadic);
            if (!paramList) {
                return NULL;
            }
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

    if (isKNR) {
        if (!parseOldStyleParamDeclarations(parser, knrParams, knrCount)) {
            free(knrParams);
            astAttributeListDestroy(funcAttrs, funcAttrCount);
            free(funcAttrs);
            return NULL;
        }
        if (parser->currentToken.type != TOKEN_LBRACE) {
            printParseError("Expected function body after old-style parameter declarations", parser);
            free(knrParams);
            astAttributeListDestroy(funcAttrs, funcAttrCount);
            free(funcAttrs);
            return NULL;
        }
        paramList = buildKNRParamDecls(knrParams, knrCount);
        if (!paramList && knrCount > 0) {
            free(knrParams);
            astAttributeListDestroy(funcAttrs, funcAttrCount);
            free(funcAttrs);
            printParseError("Failed to build parameter list", parser);
            return NULL;
        }
        paramCount = knrCount;
        isVariadic = false;
        free(knrParams);
    }

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
        parsedTypeAdjustArrayParameter(&decl.type);

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
