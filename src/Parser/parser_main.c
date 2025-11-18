#include "parser_main.h"
#include "Parser/Helpers/parser_helpers.h"
#include "parser_func.h"
#include "parser_preproc.h"
#include "parser_stmt.h"
#include "Parser/Helpers/parser_lookahead.h"
#include "Parser/Expr/parser_expr.h"
#include "parser_decl.h"


//              MAIN BLOCKS

// large blocks
ASTNode* parseProgram(Parser* parser) {
    size_t capacity = 4, count = 0;
    ASTNode** statements = malloc(capacity * sizeof(*statements));
    if (!statements) {
        fprintf(stderr, "Error: Memory allocation failed for program statements.\n");
        return NULL;
    }

    while (parser->currentToken.type != TOKEN_EOF) {
        PARSER_DEBUG_PRINTF("DEBUG: Starting new statement with token '%s' (line %d)\n",
               parser->currentToken.value, parser->currentToken.line);

        if (looksLikeTypeDeclaration(parser)) {
            ASTNode* decl = handleTypeOrFunctionDeclaration(parser);
            if (!decl) { 
                free(statements); 
                return NULL; 
            }

            if (count >= capacity) {
                size_t new_cap = capacity * 2;
                ASTNode** tmp = realloc(statements, new_cap * sizeof(*tmp));
                if (!tmp) { 
                    fprintf(stderr, "Error: Memory reallocation failed (declaration path).\n");
                    free(statements); 
                    return NULL; 
                }
                statements = tmp;
                capacity = new_cap;
            }

            statements[count++] = decl;
            continue;
        }

        ASTNode* stmt = parseStatement(parser);
        PARSER_DEBUG_PRINTF("DEBUG: Statement finished at token '%s' (line %d)\n",
               parser->currentToken.value, parser->currentToken.line);

        if (!stmt) {
            fprintf(stderr, "Error: invalid statement at line %d\n", parser->currentToken.line);
            free(statements);
            return NULL;
        }

        if (count >= capacity) {
            size_t new_cap = capacity * 2;
            ASTNode** tmp = realloc(statements, new_cap * sizeof(*tmp));
            if (!tmp) {
                fprintf(stderr, "Error: Memory reallocation failed while parsing program (statement path).\n");
                free(statements);
                return NULL;
            }
            statements = tmp;
            capacity = new_cap;
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
    PARSER_DEBUG_PRINTF("DEBUG: Current Token: %s at line %d\n",
           parser->currentToken.value, parser->currentToken.line);

    // --- Block Statement ---
    if (parser->currentToken.type == TOKEN_LBRACE) {
        return parseBlock(parser);
    }

    // --- Preprocessor Directive ---
    if (isPreprocessorToken(parser->currentToken.type)) {
        return handlePreprocessorDirectives(parser);
    }

    // --- Control Flow (if, for, while, return, etc.) ---
    ASTNode* controlStmt = handleControlStatements(parser);
    if (controlStmt) return controlStmt;

    // --- Label ---
    if (parser->currentToken.type == TOKEN_IDENTIFIER &&
        peekNextToken(parser).type == TOKEN_COLON) {
        return parseLabel(parser);
    }

    // --- Type-based Declaration (function/variable) ---
    if (parser->currentToken.type != TOKEN_LPAREN &&  // Prevent misfire on cast/group
        looksLikeTypeDeclaration(parser)) {
        PARSER_DEBUG_PRINTF("DEBUG: Detected type-based declaration\n");
        ASTNode* decl = handleTypeOrFunctionDeclaration(parser);
        if (decl) return decl;
    }

    // --- Expression or Assignment ---
    return handleExpressionOrAssignment(parser);
}
