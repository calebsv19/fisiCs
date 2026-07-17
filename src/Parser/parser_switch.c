// SPDX-License-Identifier: Apache-2.0

#include "parser_switch.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Parser/Expr/parser_expr.h"
#include "parser_main.h"

#include <stdio.h>
#include <stdlib.h>

ASTNode* parseSwitchStatement(Parser* parser) {  
    if (parser->currentToken.type != TOKEN_SWITCH) {
        printParseError("'switch'", parser);
        return NULL;
    }
    advance(parser); // Consume 'switch'
                
    if (parser->currentToken.type != TOKEN_LPAREN) {
        printParseError("'('", parser);
        return NULL;
    }
    advance(parser); // Consume '('
             
    // Parse switch condition (expression inside parentheses)
    ASTNode* condition = parseExpression(parser);
    if (!condition) {
        printParseError("valid expression", parser);
        return NULL;
    }   
            
    if (parser->currentToken.type != TOKEN_RPAREN) {
        printParseError("expected ')' after switch condition", parser);
        if (parser->currentToken.type == TOKEN_LBRACE) {
            int depth = 0;
            do {
                if (parser->currentToken.type == TOKEN_LBRACE) {
                    depth++;
                } else if (parser->currentToken.type == TOKEN_RBRACE) {
                    depth--;
                }
                advance(parser);
            } while (depth > 0 && parser->currentToken.type != TOKEN_EOF);
            return createBlockNode(NULL, 0);
        }
        return NULL;
    }
    advance(parser); // Consume ')'
                    
    if (parser->currentToken.type != TOKEN_LBRACE) {
        if (parser->currentToken.type == TOKEN_RBRACE ||
            parser->currentToken.type == TOKEN_EOF) {
            printParseError("expected statement after switch condition", parser);
            return NULL;
        }
        parser->switchDepth++;
        ASTNode* body = parseStatement(parser);
        parser->switchDepth--;
        if (!body) {
            return NULL;
        }
        ASTNode** bodyList = malloc(sizeof(*bodyList));
        if (!bodyList) {
            return NULL;
        }
        bodyList[0] = body;
        ASTNode* switchNode = createSwitchNode(condition, bodyList);
        if (!switchNode) {
            free(bodyList);
            return NULL;
        }
        switchNode->switchStmt.caseListSize = 1;
        return switchNode;
    }
    advance(parser); // Consume '{'

    parser->switchDepth++;
                    
    // Dynamic array for case nodes
    ASTNode** caseList = malloc(4 * sizeof(ASTNode*));
    size_t caseListSize = 0, caseListCapacity = 4;
    while (parser->currentToken.type != TOKEN_RBRACE && parser->currentToken.type != TOKEN_EOF) {
        if (parser->currentToken.type == TOKEN_CASE || parser->currentToken.type == TOKEN_DEFAULT) {
            ASTNode* caseValue = NULL;
            if (parser->currentToken.type == TOKEN_CASE) {
                advance(parser);
                caseValue = parseExpression(parser);
                if (!caseValue) {
                    printf("Error: Invalid case value at line %d\n", parser->currentToken.line);
                    parser->switchDepth--;
                    return NULL;
                }
            } else { // Default case
                advance(parser);
            }
             
            if (parser->currentToken.type != TOKEN_COLON) {
                printParseError("':' after case value", parser);
                parser->switchDepth--;
                return NULL;
            }
            advance(parser); // Consume ':'
            
            // Dynamic array for case body statements
            ASTNode** caseBody = malloc(4 * sizeof(ASTNode*));
            size_t caseBodySize = 0, caseBodyCapacity = 4;
            
            while (parser->currentToken.type != TOKEN_CASE &&
                   parser->currentToken.type != TOKEN_DEFAULT &&
                   parser->currentToken.type != TOKEN_RBRACE && 
                   parser->currentToken.type != TOKEN_EOF) {    
                ASTNode* stmt = parseStatement(parser);
                if (!stmt) {
                    printf("Error: Invalid statement inside case block at line %d\n", 
					parser->currentToken.line);
                    parser->switchDepth--;
                    return NULL;
                }
                 
                // Store statement in caseBody array
                if (caseBodySize >= caseBodyCapacity) {
                    caseBodyCapacity *= 2;
                    caseBody = realloc(caseBody, caseBodyCapacity * sizeof(ASTNode*));
                }
                caseBody[caseBodySize++] = stmt;
            }
             
            // Create and store case node
            ASTNode* caseNode = createCaseNode(caseValue, caseBody);
            if (!caseNode) {
                parser->switchDepth--;
                return NULL;
            }
            caseNode->caseStmt.caseBodySize = caseBodySize;

            // Store in caseList
            if (caseListSize >= caseListCapacity) {
                caseListCapacity *= 2;
                caseList = realloc(caseList, caseListCapacity * sizeof(ASTNode*));
            }
            caseList[caseListSize++] = caseNode;
        } else {
            /* A switch body is an ordinary compound statement whose labels may
               occur inside nested control statements (Duff-style forms). Keep
               those structural statements in source order so semantic analysis
               and codegen can discover the case nodes they own. */
            ASTNode* stmt = parseStatement(parser);
            if (!stmt) {
                printf("Error: Invalid statement inside switch body at line %d\n",
                       parser->currentToken.line);
                parser->switchDepth--;
                return NULL;
            }
            if (caseListSize >= caseListCapacity) {
                caseListCapacity *= 2;
                caseList = realloc(caseList, caseListCapacity * sizeof(ASTNode*));
            }
            caseList[caseListSize++] = stmt;
        }
    }
     
    if (parser->currentToken.type != TOKEN_RBRACE) {
        printParseError("'}'", parser);
        parser->switchDepth--;
        return NULL;
    }
    advance(parser); // Consume '}'

    parser->switchDepth--;
    
    ASTNode* switchNode = createSwitchNode(condition, caseList);
    if (!switchNode) {
        return NULL;
    }
    switchNode->switchStmt.caseListSize = caseListSize;
    return switchNode;
}



void linkStatements(ASTNode* caseBody, ASTNode* stmt) {
    ASTNode* lastStatement = caseBody;
    if (!caseBody) {
        caseBody = stmt;
    } else {
        while (lastStatement->nextStmt) {
            lastStatement = lastStatement->nextStmt;
        }
        lastStatement->nextStmt = stmt;
    }
}  
