// SPDX-License-Identifier: Apache-2.0

#include "Parser/Helpers/parser_lookahead.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Parser/Helpers/parser_attributes.h"
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

static const char* parserTypeDeclSiteScopeName(ParserTypeDeclLookaheadSite site) {
    switch (site) {
        case PARSER_TYPE_DECL_SITE_PROGRAM_TOPLEVEL:
            return "parser_type_decl_site_program_toplevel";
        case PARSER_TYPE_DECL_SITE_STATEMENT:
            return "parser_type_decl_site_statement";
        case PARSER_TYPE_DECL_SITE_FOR_INIT:
            return "parser_type_decl_site_for_init";
        case PARSER_TYPE_DECL_SITE_SYNC_RECOVERY:
            return "parser_type_decl_site_sync_recovery";
        case PARSER_TYPE_DECL_SITE_BUILTIN_OFFSETOF:
            return "parser_type_decl_site_builtin_offsetof";
        case PARSER_TYPE_DECL_SITE_BUILTIN_VA_ARG:
            return "parser_type_decl_site_builtin_va_arg";
        default:
            return "parser_type_decl_site_unknown";
    }
}

static void parserRecordTypeDeclSiteCount(ParserTypeDeclLookaheadSite site, bool result) {
    switch (site) {
        case PARSER_TYPE_DECL_SITE_PROGRAM_TOPLEVEL:
            profiler_record_value("parser_count_type_decl_site_program_toplevel", 1);
            profiler_record_value(result
                                      ? "parser_count_type_decl_site_program_toplevel_positive"
                                      : "parser_count_type_decl_site_program_toplevel_negative",
                                  1);
            break;
        case PARSER_TYPE_DECL_SITE_STATEMENT:
            profiler_record_value("parser_count_type_decl_site_statement", 1);
            profiler_record_value(result
                                      ? "parser_count_type_decl_site_statement_positive"
                                      : "parser_count_type_decl_site_statement_negative",
                                  1);
            break;
        case PARSER_TYPE_DECL_SITE_FOR_INIT:
            profiler_record_value("parser_count_type_decl_site_for_init", 1);
            profiler_record_value(result
                                      ? "parser_count_type_decl_site_for_init_positive"
                                      : "parser_count_type_decl_site_for_init_negative",
                                  1);
            break;
        case PARSER_TYPE_DECL_SITE_SYNC_RECOVERY:
            profiler_record_value("parser_count_type_decl_site_sync_recovery", 1);
            profiler_record_value(result
                                      ? "parser_count_type_decl_site_sync_recovery_positive"
                                      : "parser_count_type_decl_site_sync_recovery_negative",
                                  1);
            break;
        case PARSER_TYPE_DECL_SITE_BUILTIN_OFFSETOF:
            profiler_record_value("parser_count_type_decl_site_builtin_offsetof", 1);
            profiler_record_value(result
                                      ? "parser_count_type_decl_site_builtin_offsetof_positive"
                                      : "parser_count_type_decl_site_builtin_offsetof_negative",
                                  1);
            break;
        case PARSER_TYPE_DECL_SITE_BUILTIN_VA_ARG:
            profiler_record_value("parser_count_type_decl_site_builtin_va_arg", 1);
            profiler_record_value(result
                                      ? "parser_count_type_decl_site_builtin_va_arg_positive"
                                      : "parser_count_type_decl_site_builtin_va_arg_negative",
                                  1);
            break;
        default:
            profiler_record_value("parser_count_type_decl_site_unknown", 1);
            break;
    }
}

ParenthesizedTypeFormKind parserProbeParenthesizedTypeForm(Parser* parser) {
    if (!parser) {
        return PARENTHESIZED_TYPE_FORM_NONE;
    }

    Parser probe = cloneParserWithFreshLexer(parser);
    ParsedType looked = parseTypeCtx(&probe, TYPECTX_Strict);
    ParenthesizedTypeFormKind result = PARENTHESIZED_TYPE_FORM_NONE;

    if (looked.kind != TYPE_INVALID) {
        consumeAbstractDeclarator(&probe);
        if (probe.currentToken.type == TOKEN_RPAREN) {
            advance(&probe);
            if (probe.currentToken.type == TOKEN_LBRACE) {
                result = PARENTHESIZED_TYPE_FORM_COMPOUND_LITERAL;
            } else if (isValidExpressionStart(probe.currentToken.type)) {
                result = PARENTHESIZED_TYPE_FORM_CAST;
            }
        }
    }

    parsedTypeFree(&looked);
    freeParserClone(&probe);
    return result;
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

    if (parser->currentToken.type == TOKEN_IDENTIFIER &&
        !parserLookaheadStartsAttribute(parser)) {
        profiler_record_value("parser_count_type_decl_fast_identifier_negative", 1);
        profiler_end(scope);
        return false;
    }

    profiler_record_value("parser_count_type_decl_clone_probe", 1);
    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        profiler_record_value("parser_count_type_decl_clone_probe_identifier", 1);
    } else {
        profiler_record_value("parser_count_type_decl_clone_probe_other", 1);
    }
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
    if (result) {
        profiler_record_value("parser_count_type_decl_clone_probe_positive", 1);
    } else {
        profiler_record_value("parser_count_type_decl_clone_probe_negative", 1);
        if (parser->currentToken.type == TOKEN_IDENTIFIER) {
            profiler_record_value("parser_count_type_decl_clone_probe_identifier_negative", 1);
        }
    }
    profiler_end(scope);
    return result;
}

bool parserLooksLikeTypeDeclarationAt(Parser* parser, ParserTypeDeclLookaheadSite site) {
    ProfilerScope scope = profiler_begin(parserTypeDeclSiteScopeName(site));
    bool result = looksLikeTypeDeclaration(parser);
    parserRecordTypeDeclSiteCount(site, result);
    profiler_end(scope);
    return result;
}





bool looksLikeCastType(Parser* parser) {
    ProfilerScope scope = profiler_begin("parser_lookahead_cast_type");
    profiler_record_value("parser_count_cast_type_probe", 1);
    bool ok = (parserProbeParenthesizedTypeForm(parser) == PARENTHESIZED_TYPE_FORM_CAST);
    profiler_end(scope);
    return ok;
}




bool looksLikeCompoundLiteral(Parser* parser) {
    ProfilerScope scope = profiler_begin("parser_lookahead_compound_literal");
    profiler_record_value("parser_count_compound_literal_probe", 1);
    bool ok = (parserProbeParenthesizedTypeForm(parser) == PARENTHESIZED_TYPE_FORM_COMPOUND_LITERAL);
    profiler_end(scope);
    return ok;
}
