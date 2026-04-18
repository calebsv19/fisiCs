// SPDX-License-Identifier: Apache-2.0

#include "Parser/Helpers/parser_lookahead.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Parser/Expr/parser_expr_pratt.h"
#include "parser_decl.h"
#include "Utils/profiler.h"
#include <string.h>

static bool isAlignasToken(const Token* tok) {
    if (!tok || tok->type != TOKEN_IDENTIFIER || !tok->value) return false;
    return strcmp(tok->value, "alignas") == 0 || strcmp(tok->value, "_Alignas") == 0;
}

static bool isAtomicKeywordToken(const Token* tok) {
    return tok && tok->type == TOKEN_ATOMIC;
}

void printParserState(const char* label, Parser* parser) {
    printf("\n=== PARSER STATE: %s ===\n", label);
    printf("currentToken:      %-10s at line %d\n", parser->currentToken.value, parser->currentToken.line);
    printf("nextToken:         %-10s at line %d\n", parser->nextToken.value, parser->nextToken.line);
    printf("nextNextToken:     %-10s at line %d\n", parser->nextNextToken.value, parser->nextNextToken.line);
    printf("nextNextNextToken: %-10s at line %d\n", parser->nextNextNextToken.value, parser->nextNextNextToken.line);
    if (parser->tokenBuffer) {
        printf("Token buffer size: %zu\n", parser->tokenBuffer->count);
        printf("Cursor index:      %zu\n", parser->cursor);
    }
    printf("===========================\n\n");
}


bool looksLikeTypeDeclaration(Parser* parser) {
    ProfilerScope scope = profiler_begin("parser_lookahead_type_decl");
    if (parser->currentToken.type == TOKEN_TYPEDEF) {
        profiler_record_value("parser_count_type_decl_typedef_token", 1);
        profiler_end(scope);
        return false; // handled explicitly via parseTypedef
    }
    if (isAlignasToken(&parser->currentToken)) {
        profiler_record_value("parser_count_type_decl_fast_alignas", 1);
        profiler_end(scope);
        return true;
    }
    if (isAtomicKeywordToken(&parser->currentToken)) {
        profiler_record_value("parser_count_type_decl_fast_atomic", 1);
        profiler_end(scope);
        return true;
    }

    // Fast path: obvious type-start tokens
    switch (parser->currentToken.type) {
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_DOUBLE:
        case TOKEN_CHAR:
        case TOKEN_LONG:
        case TOKEN_SHORT:
        case TOKEN_SIGNED:
        case TOKEN_UNSIGNED:
        case TOKEN_VOID:
        case TOKEN_BOOL:
        case TOKEN_STRUCT:
        case TOKEN_UNION:
        case TOKEN_ENUM:
        case TOKEN_CONST:
        case TOKEN_VOLATILE:
        case TOKEN_RESTRICT:
        case TOKEN_INLINE:
        case TOKEN_STATIC:
        case TOKEN_EXTERN:
        case TOKEN_AUTO:
        case TOKEN_REGISTER:
        case TOKEN_THREAD_LOCAL:
        case TOKEN_ATOMIC:
            profiler_record_value("parser_count_type_decl_fast_token", 1);
            profiler_end(scope);
            return true;
        default:
            break;
    }

    if (parser->currentToken.type == TOKEN_IDENTIFIER &&
        parser->ctx &&
        parserIsTypedefVisible(parser, parser->currentToken.value)) {
        profiler_record_value("parser_count_type_decl_typedef_visible", 1);
        profiler_end(scope);
        return true;
    }

    profiler_record_value("parser_count_type_decl_clone_probe", 1);
    Parser temp = cloneParserWithFreshLexer(parser);
    ParsedType probe = parseTypeCtx(&temp, TYPECTX_Strict);
    bool result = false;
    if (probe.kind != TYPE_INVALID) {
        if (temp.currentToken.type == TOKEN_IDENTIFIER) {
            result = true;
        } else if (temp.currentToken.type == TOKEN_LPAREN) {
            result = true;
        }
    }

    parsedTypeFree(&probe);
    freeParserClone(&temp);
    profiler_end(scope);
    return result;
}





bool looksLikeCastType(Parser* parser) {
    ProfilerScope scope = profiler_begin("parser_lookahead_cast_type");
    profiler_record_value("parser_count_cast_type_probe", 1);
    // Precondition: '(' was consumed by Pratt before nud('('),
    // so parser->currentToken is *after* '('.
    Parser probe = cloneParserWithFreshLexer(parser);

    // Parse a base type (primitive/typedef/struct/etc.)
    ParsedType t = parseTypeCtx(&probe, TYPECTX_Strict);
    if (t.kind == TYPE_INVALID) {
        parsedTypeFree(&t);
        freeParserClone(&probe);
        return false;
    }

    // Swallow abstract declarators: (*)(...), [N], (...)
    consumeAbstractDeclarator(&probe);

    // We must be at the closing ')'
    if (probe.currentToken.type != TOKEN_RPAREN) {
        parsedTypeFree(&t);
        freeParserClone(&probe);
        return false;
    }

    // (Optional sanity) after ')', should be a valid expr start
    advance(&probe); // consume ')'
    bool ok = isValidExpressionStart(probe.currentToken.type);

    parsedTypeFree(&t);
    parsedTypeFree(&t);
    freeParserClone(&probe);
    profiler_end(scope);
    return ok;
}




bool looksLikeCompoundLiteral(Parser* parser) {
    ProfilerScope scope = profiler_begin("parser_lookahead_compound_literal");
    profiler_record_value("parser_count_compound_literal_probe", 1);
    // Pre: '(' already consumed by nud('('), so parser->currentToken is after '('
    Parser probe = cloneParserWithFreshLexer(parser);

    ParsedType t = parseTypeCtx(&probe, TYPECTX_Strict);
    if (t.kind == TYPE_INVALID) { parsedTypeFree(&t); freeParserClone(&probe); return false; }

    // Allow pointers/params/arrays after base type
    consumeAbstractDeclarator(&probe);

    // Must see ')'
    if (probe.currentToken.type != TOKEN_RPAREN) { parsedTypeFree(&t); freeParserClone(&probe); return false; }
    advance(&probe); // consume ')'

    // C99 compound literal: next token must be '{'
    bool ok = (probe.currentToken.type == TOKEN_LBRACE);
    parsedTypeFree(&t);
    parsedTypeFree(&t);
    freeParserClone(&probe);
    profiler_end(scope);
    return ok;
}
