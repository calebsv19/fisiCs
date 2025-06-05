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

    // --- Function Pointer Declaration ---
    if (looksLikeFunctionPointerDeclaration(parser)) {
        printf("DEBUG: Detected function pointer declaration\n");
        return parseFunctionPointerDeclaration(parser);
    }

    // --- Type-based Declaration (function/variable) ---
    if (parser->currentToken.type != TOKEN_LPAREN &&  // Prevent misfire on cast/group
        looksLikeTypeDeclaration(parser)) {
        printf("DEBUG: Detected type-based declaration\n");
        ASTNode* decl = handleTypeOrFunctionDeclaration(parser);
        if (decl) return decl;
    }

    // --- Expression or Assignment ---
    return handleExpressionOrAssignment(parser);
}

