// SPDX-License-Identifier: Apache-2.0

#include "parser_main.h"
#include "Parser/Helpers/parser_helpers.h"
#include "parser_func.h"
#include "parser_stmt.h"
#include "Parser/Helpers/parser_lookahead.h"
#include "Parser/Expr/parser_expr.h"
#include "parser_decl.h"
#include "Parser/Helpers/parser_attributes.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


//              MAIN BLOCKS
static void consume_line(Parser* parser, int line) {
    while (parser->currentToken.type != TOKEN_EOF &&
           parser->currentToken.line == line) {
        advance(parser);
    }
}

static bool expectTokenLocal(Parser* parser, TokenType type, const char* message) {
    if (!parser) return false;
    if (parser->currentToken.type != type) {
        printParseError(message ? message : "Unexpected token", parser);
        return false;
    }
    advance(parser);
    return true;
}

static char* concat_tokens_on_line(Parser* parser, int line) {
    size_t cap = 64;
    size_t len = 0;
    char* buf = malloc(cap);
    if (!buf) return NULL;
    buf[0] = '\0';
    int first = 1;
    size_t cursor = parser->cursor + 1;
    while (cursor < parser->tokenBuffer->count) {
        const Token* tok = token_buffer_peek(parser->tokenBuffer, cursor);
        if (!tok || tok->type == TOKEN_EOF || tok->line != line) break;
        const char* text = tok->value ? tok->value : "";
        size_t tlen = strlen(text);
        size_t need = len + tlen + (first ? 0 : 1) + 1;
        if (need > cap) {
            cap = need * 2;
            char* nb = realloc(buf, cap);
            if (!nb) { free(buf); return NULL; }
            buf = nb;
        }
        if (!first) {
            buf[len++] = ' ';
        }
        memcpy(buf + len, text, tlen);
        len += tlen;
        buf[len] = '\0';
        first = 0;
        cursor++;
    }
    return buf;
}

static ASTNode* parse_preserved_include(Parser* parser) {
    int line = parser->currentToken.line;
    advance(parser); // consume include
    if (parser->currentToken.type == TOKEN_EOF) return NULL;

    bool isSystem = false;
    char pathBuf[512];
    pathBuf[0] = '\0';

    if (parser->currentToken.type == TOKEN_STRING) {
        strncpy(pathBuf, parser->currentToken.value ? parser->currentToken.value : "", sizeof(pathBuf) - 1);
        pathBuf[sizeof(pathBuf) - 1] = '\0';
        isSystem = false;
        advance(parser);
    } else if (parser->currentToken.type == TOKEN_LESS) {
        size_t len = 0;
        advance(parser);
        while (parser->currentToken.type != TOKEN_EOF &&
               parser->currentToken.line == line &&
               parser->currentToken.type != TOKEN_GREATER) {
            const char* text = parser->currentToken.value ? parser->currentToken.value : "";
            size_t tlen = strlen(text);
            if (len + tlen + 1 >= sizeof(pathBuf)) break;
            memcpy(pathBuf + len, text, tlen);
            len += tlen;
            pathBuf[len] = '\0';
            advance(parser);
        }
        isSystem = true;
        if (parser->currentToken.type == TOKEN_GREATER) {
            advance(parser);
        }
    } else {
        consume_line(parser, line);
        return NULL;
    }

    ASTNode* node = createIncludeDirectiveNode(pathBuf, isSystem);
    consume_line(parser, line);
    return node;
}

static ASTNode* parse_preserved_define(Parser* parser) {
    int line = parser->currentToken.line;
    advance(parser); // consume define
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        consume_line(parser, line);
        return NULL;
    }
    const char* name = parser->currentToken.value;
    advance(parser);
    char* value = concat_tokens_on_line(parser, line);
    ASTNode* node = createDefineDirectiveNode(name ? name : "", value && value[0] ? value : NULL);
    free(value);
    consume_line(parser, line);
    return node;
}

static ASTNode* parse_preserved_conditional(Parser* parser, bool negate) {
    int line = parser->currentToken.line;
    advance(parser); // consume ifdef/ifndef
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        consume_line(parser, line);
        return NULL;
    }
    const char* name = parser->currentToken.value;
    ASTNode* node = createConditionalDirectiveNode(name ? name : "", negate);
    consume_line(parser, line);
    return node;
}

static ASTNode* parse_preserved_endif(Parser* parser) {
    int line = parser->currentToken.line;
    advance(parser);
    consume_line(parser, line);
    return createEndifDirectiveNode();
}

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

        if (parser->preserveDirectives) {
            ASTNode* ppNode = NULL;
            switch (parser->currentToken.type) {
                case TOKEN_INCLUDE:
                    ppNode = parse_preserved_include(parser);
                    break;
                case TOKEN_DEFINE:
                    ppNode = parse_preserved_define(parser);
                    break;
                case TOKEN_IFDEF:
                    ppNode = parse_preserved_conditional(parser, false);
                    break;
                case TOKEN_IFNDEF:
                    ppNode = parse_preserved_conditional(parser, true);
                    break;
                case TOKEN_ENDIF:
                    ppNode = parse_preserved_endif(parser);
                    break;
                default:
                    break;
            }
            if (ppNode) {
                if (count >= capacity) {
                    size_t new_cap = capacity * 2;
                    ASTNode** tmp = realloc(statements, new_cap * sizeof(*tmp));
                    if (!tmp) {
                        fprintf(stderr, "Error: Memory reallocation failed (pp directive path).\n");
                        free(statements);
                        return NULL;
                    }
                    statements = tmp;
                    capacity = new_cap;
                }
                statements[count++] = ppNode;
                continue;
            }
        }

        if (parser->currentToken.type == TOKEN_STRUCT) {
            Token n = peekNextToken(parser);
            Token a = peekTwoTokensAhead(parser);
            if ((n.type == TOKEN_IDENTIFIER && a.type == TOKEN_LBRACE) ||
                n.type == TOKEN_LBRACE) {
                ASTNode* def = parseStructDefinition(parser);
                if (def) {
                    if (count >= capacity) {
                        size_t new_cap = capacity * 2;
                        ASTNode** tmp = realloc(statements, new_cap * sizeof(*tmp));
                        if (!tmp) { free(statements); return NULL; }
                        statements = tmp;
                        capacity = new_cap;
                    }
                    statements[count++] = def;
                    continue;
                }
            }
        }

        if (parser->currentToken.type == TOKEN_UNION) {
            Token n = peekNextToken(parser);
            Token a = peekTwoTokensAhead(parser);
            if ((n.type == TOKEN_IDENTIFIER && a.type == TOKEN_LBRACE) ||
                n.type == TOKEN_LBRACE) {
                ASTNode* def = parseUnionDefinition(parser);
                if (def) {
                    if (count >= capacity) {
                        size_t new_cap = capacity * 2;
                        ASTNode** tmp = realloc(statements, new_cap * sizeof(*tmp));
                        if (!tmp) { free(statements); return NULL; }
                        statements = tmp;
                        capacity = new_cap;
                    }
                    statements[count++] = def;
                    continue;
                }
            }
        }

        if (parser->currentToken.type == TOKEN_ENUM) {
            Token n = peekNextToken(parser);
            Token a = peekTwoTokensAhead(parser);
            if (n.type == TOKEN_LBRACE ||
                (n.type == TOKEN_IDENTIFIER && a.type == TOKEN_LBRACE)) {
                ASTNode* def = parseEnumDefinition(parser);
                if (def) {
                    if (count >= capacity) {
                        size_t new_cap = capacity * 2;
                        ASTNode** tmp = realloc(statements, new_cap * sizeof(*tmp));
                        if (!tmp) { free(statements); return NULL; }
                        statements = tmp;
                        capacity = new_cap;
                    }
                    statements[count++] = def;
                    continue;
                }
            }
        }

        if (parser->currentToken.type == TOKEN_TYPEDEF) {
            ASTNode* def = parseTypedef(parser);
            if (def) {
                if (count >= capacity) {
                    size_t new_cap = capacity * 2;
                    ASTNode** tmp = realloc(statements, new_cap * sizeof(*tmp));
                    if (!tmp) { free(statements); return NULL; }
                    statements = tmp;
                    capacity = new_cap;
                }
                statements[count++] = def;
            } else {
                parserSyncToDeclarationStart(parser);
            }
            continue;
        }

        if (parserLooksLikeTypeDeclarationAt(parser, PARSER_TYPE_DECL_SITE_PROGRAM_TOPLEVEL)) {
            ASTNode* decl = handleTypeOrFunctionDeclaration(parser);
            if (!decl) { 
                parserSyncToDeclarationStart(parser);
                continue;
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
            fprintf(stderr,
                    "Error: invalid statement at line %d (token=%s)\n",
                    parser->currentToken.line,
                    parser->currentToken.value ? parser->currentToken.value : "<null>");
            parserSyncToStatementEnd(parser);
            if (parser->currentToken.type == TOKEN_RBRACE) {
                advance(parser);
            }
            continue;
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
    parserPushOrdinaryScope(parser);
    advance(parser);  // consume '{'
    size_t capacity = 4, count = 0; 
    ASTNode **statements = malloc(capacity * sizeof(ASTNode*));
    while (parser->currentToken.type != TOKEN_RBRACE && parser->currentToken.type != TOKEN_EOF) {
        ASTNode* stmt = parseStatement(parser);
        if (!stmt) {
            printf("Error: invalid statement inside block at line %d\n", parser->currentToken.line);
            parserSyncToStatementEnd(parser);
            if (parser->currentToken.type == TOKEN_RBRACE || parser->currentToken.type == TOKEN_EOF) {
                break;
            }
            continue;
        }
        if (count >= capacity) {
            capacity *= 2;
            statements = realloc(statements, capacity * sizeof(ASTNode*));
            if (!statements) {
                printf("Error: Memory reallocation failed while parsing block.\n");
                parserPopOrdinaryScope(parser);
                return NULL;
            }
        }
        statements[count++] = stmt;
    }
    if (parser->currentToken.type != TOKEN_RBRACE) {
        printf("Error: expected '}' at line %d\n", parser->currentToken.line);
    } else {
        advance(parser);  // consume '}'
    }
    ASTNode* block = createBlockNode(statements, count);
    parserPopOrdinaryScope(parser);
    return block;
}

ASTNode* parseStatement(Parser* parser) {
    PARSER_DEBUG_PRINTF("DEBUG: Current Token: %s at line %d\n",
           parser->currentToken.value, parser->currentToken.line);

    if (parser->currentToken.type == TOKEN_ASM) {
        return parseAsmStatement(parser);
    }

    if (parser->currentToken.type == TOKEN_STATIC_ASSERT) {
        Token staticAssertTok = parser->currentToken;
        advance(parser); // consume _Static_assert
        if (!expectTokenLocal(parser, TOKEN_LPAREN, "Expected '(' after _Static_assert")) {
            return NULL;
        }
        ASTNode* expr = parseAssignmentExpression(parser);
        if (!expr) {
            printParseError("Expected constant expression in _Static_assert", parser);
            return NULL;
        }
        ASTNode* message = NULL;
        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser);
            if (parser->currentToken.type == TOKEN_STRING) {
                Token messageTok = parser->currentToken;
                message = createStringLiteralNode(parser->currentToken.value);
                if (message) {
                    astNodeSetProvenance(message, &messageTok);
                }
                advance(parser);
            } else {
                printParseError("Expected string literal in _Static_assert", parser);
                return NULL;
            }
        }
        if (!expectTokenLocal(parser, TOKEN_RPAREN, "Expected ')' after _Static_assert")) {
            return NULL;
        }
        if (!expectTokenLocal(parser, TOKEN_SEMICOLON, "Expected ';' after _Static_assert")) {
            return NULL;
        }
        ASTNode* node = createStaticAssertNode(expr, message);
        if (node) {
            astNodeSetProvenance(node, &staticAssertTok);
        }
        return node;
    }

    size_t stmtAttrCount = 0;
    ASTAttribute** stmtAttrs = parserParseAttributeSpecifiers(parser, &stmtAttrCount);
    ASTNode* parsed = NULL;

    // --- Block Statement ---
    if (parser->currentToken.type == TOKEN_LBRACE) {
        parsed = parseBlock(parser);
        goto attach_attrs;
    }

    // --- Empty statement ---
    if (parser->currentToken.type == TOKEN_SEMICOLON) {
        advance(parser);
        parsed = createBlockNode(NULL, 0);
        goto attach_attrs;
    }

    // --- Control Flow (if, for, while, return, etc.) ---
    TokenType controlStartTok = parser->currentToken.type;
    ASTNode* controlStmt = handleControlStatements(parser);
    if (controlStmt) {
        parsed = controlStmt;
        goto attach_attrs;
    }
    // Do-while recovery can leave us on a non-expression token (for example
    // 'return') when the trailing ';' is missing. In that case, falling into
    // expression parsing creates duplicate parser diagnostics for the same spot.
    if (controlStartTok == TOKEN_DO && !isValidExpressionStart(parser->currentToken.type)) {
        parsed = NULL;
        goto attach_attrs;
    }

    // --- Case labels inside switch bodies (Duff's device, nested blocks) ---
    if (parser->switchDepth > 0 &&
        (parser->currentToken.type == TOKEN_CASE || parser->currentToken.type == TOKEN_DEFAULT)) {
        bool isDefault = (parser->currentToken.type == TOKEN_DEFAULT);
        advance(parser);
        if (!isDefault) {
            ASTNode* caseExpr = parseExpression(parser);
            if (!caseExpr) {
                printParseError("case label expression", parser);
                return NULL;
            }
        }
        if (parser->currentToken.type != TOKEN_COLON) {
            printParseError("':' after case label", parser);
            return NULL;
        }
        advance(parser);
        parsed = parseStatement(parser);
        goto attach_attrs;
    }

    // --- Label ---
    if (parser->currentToken.type == TOKEN_IDENTIFIER &&
        peekNextToken(parser).type == TOKEN_COLON) {
        parsed = parseLabel(parser);
        goto attach_attrs;
    }

    // --- Type-based Declaration (function/variable) ---
    if (parser->currentToken.type != TOKEN_LPAREN &&  // Prevent misfire on cast/group
        parserLooksLikeTypeDeclarationAt(parser, PARSER_TYPE_DECL_SITE_STATEMENT)) {
        PARSER_DEBUG_PRINTF("DEBUG: Detected type-based declaration\n");
        parsed = handleTypeOrFunctionDeclaration(parser);
        if (parsed) goto attach_attrs;
    }

    // --- Expression or Assignment ---
    parsed = handleExpressionOrAssignment(parser);

attach_attrs:
    if (stmtAttrCount > 0) {
        if (parsed) {
            astNodeAppendAttributes(parsed, stmtAttrs, stmtAttrCount);
        } else {
            astAttributeListDestroy(stmtAttrs, stmtAttrCount);
            free(stmtAttrs);
        }
    } else {
        free(stmtAttrs);
    }
    return parsed;
}
