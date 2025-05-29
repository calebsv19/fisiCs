#include "parser_main.h"
#include "parser_helpers.h"
#include "parser_func.h"
#include "parser_preproc.h"
#include "parser_stmt.h"
#include "parser_lookahead.h"
#include "parser_expr.h"
#include "parser_decl.h"


//              MAIN BLOCKS


// large blocks
ASTNode* parseProgram(Parser* parser) {
    size_t capacity = 4, count = 0;
    ASTNode** statements = malloc(capacity * sizeof(ASTNode*));

    if (!statements) {
        printf("Error: Memory allocation failed for program statements.\n");
        return NULL;
    }

    while (parser->currentToken.type != TOKEN_EOF) {
        printf("DEBUG: Starting new statement with token '%s' (line %d)\n",
                   parser->currentToken.value, parser->currentToken.line);

        ASTNode* stmt = parseStatement(parser);
        printf("DEBUG: Statement finished at token '%s' (line %d)\n",
                       parser->currentToken.value, parser->currentToken.line);

        if (!stmt) {
            printf("Error: invalid statement at line %d\n", parser->currentToken.line);
            free(statements);
            return NULL;
        }

        // Expand storage if needed
        if (count >= capacity) {
            capacity *= 2;
            statements = realloc(statements, capacity * sizeof(ASTNode*));
            if (!statements) {
                printf("Error: Memory reallocation failed while parsing program.\n");
                return NULL;
            }
        }

        statements[count++] = stmt;
    }

    return createProgramNode(statements, count);
}


ASTNode* parseBlock(Parser* parser) {
    if (parser->currentToken.type != TOKEN_LBRACE) {
        printf("Error: invalid block at line %d\n", parser->currentToken.line);
        return NULL;
    }
    advance(parser);  // consume '{'
    size_t capacity = 4, count = 0; 
    ASTNode **statements = malloc(capacity * sizeof(ASTNode*));
    while (parser->currentToken.type != TOKEN_RBRACE && parser->currentToken.type != TOKEN_EOF) {
        ASTNode* stmt = parseStatement(parser);
        if (!stmt) {
            printf("Error: invalid statement inside block at line %d\n", parser->currentToken.line);
            free(statements);
            return NULL;
        }
        if (count >= capacity) {
            capacity *= 2;
            statements = realloc(statements, capacity * sizeof(ASTNode*));
            if (!statements) {
                printf("Error: Memory reallocation failed while parsing block.\n");
                return NULL;
            }
        }
        statements[count++] = stmt;
    }
    if (parser->currentToken.type != TOKEN_RBRACE) {
        printf("Error: expected '}' at line %d\n", parser->currentToken.line);
        free(statements);
        return NULL;
    }
    advance(parser);  // consume '}'
    return createBlockNode(statements, count);
}

ASTNode* parseStatement(Parser* parser) {
    printf("DEBUG: Current Token: %s at line %d\n", parser->currentToken.value,
           parser->currentToken.line);
           
    // Handle block {...}
    if (parser->currentToken.type == TOKEN_LBRACE) {
        return parseBlock(parser);
    }
     
    // Handle preprocessor directives
    if (isPreprocessorToken(parser->currentToken.type)) {
        return handlePreprocessorDirectives(parser);
    }
     
    // Handle control flow statements (if, while, for, return, break, continue, etc.)
    ASTNode* controlStmt = handleControlStatements(parser);
    if (controlStmt) return controlStmt;
    
    // Handle label: syntax
    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        Token next = peekNextToken(parser);
        if (next.type == TOKEN_COLON) {
            ASTNode* labelNode = parseLabel(parser);
            if (labelNode) return labelNode;
        }
    }


     
    if (looksLikeFunctionPointerDeclaration(parser)) {
        printf("checking function pointer declaration passed\n");
        return parseFunctionPointerDeclaration(parser);
    }

     
     
    // Check for variable/function declarations
    if (looksLikeTypeDeclaration(parser)) {
        printf("checking looks like type celcaration passed\n");
        ASTNode* typeOrFunction = handleTypeOrFunctionDeclaration(parser);
        if (typeOrFunction) return typeOrFunction;
    }
     
    // Fallback: treat as a regular expression or assignment
    return handleExpressionOrAssignment(parser);
}
