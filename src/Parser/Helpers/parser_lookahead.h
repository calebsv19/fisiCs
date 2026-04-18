// SPDX-License-Identifier: Apache-2.0

#ifndef _PARSER_LOOKAHEAD_H
#define _PARSER_LOOKAHEAD_H

#include "parser.h"
#include "Parser/Helpers/parsed_type.h"

#include "AST/ast_node.h"

#include "Lexer/tokens.h"
#include "Lexer/lexer.h"

void printParserState(const char* label, Parser* parser);

typedef enum ParenthesizedTypeFormKind {
    PARENTHESIZED_TYPE_FORM_NONE = 0,
    PARENTHESIZED_TYPE_FORM_CAST,
    PARENTHESIZED_TYPE_FORM_COMPOUND_LITERAL
} ParenthesizedTypeFormKind;

typedef enum ParserTypeDeclLookaheadSite {
    PARSER_TYPE_DECL_SITE_PROGRAM_TOPLEVEL = 0,
    PARSER_TYPE_DECL_SITE_STATEMENT,
    PARSER_TYPE_DECL_SITE_FOR_INIT,
    PARSER_TYPE_DECL_SITE_SYNC_RECOVERY,
    PARSER_TYPE_DECL_SITE_BUILTIN_OFFSETOF,
    PARSER_TYPE_DECL_SITE_BUILTIN_VA_ARG
} ParserTypeDeclLookaheadSite;

// Predictive lookahead checks
bool looksLikeTypeDeclaration(Parser* parser);
bool parserLooksLikeTypeDeclarationAt(Parser* parser, ParserTypeDeclLookaheadSite site);
bool looksLikeCastType(Parser* parser);
bool looksLikeCompoundLiteral(Parser* parser);
ParenthesizedTypeFormKind parserProbeParenthesizedTypeForm(Parser* parser);


#endif // _PARSER_LOOKAHEAD_H
