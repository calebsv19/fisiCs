#include "parser_switch.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Parser/Expr/parser_expr.h"
#include "parser_main.h"

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
        printParseError("')'", parser);
        return NULL;
    }
    advance(parser); // Consume ')'
                    
    if (parser->currentToken.type != TOKEN_LBRACE) {
        printParseError("'{'", parser);
        return NULL;
    }
    advance(parser); // Consume '{'
                    
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
                    return NULL;
                }
            } else { // Default case
                advance(parser);
            }
             
            if (parser->currentToken.type != TOKEN_COLON) {
                printParseError("':' after case value", parser);
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
            printf("Error: Unexpected token '%s' inside switch statement at line %d\n",
                   parser->currentToken.value, parser->currentToken.line);
            return NULL;
        }
    }
     
    if (parser->currentToken.type != TOKEN_RBRACE) {
        printParseError("'}'", parser);
        return NULL;
    }
    advance(parser); // Consume '}'
    
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
