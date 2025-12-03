#ifndef _PARSER_HELPERS_H
#define _PARSER_HELPERS_H

#include "parser.h"
#include "Parser/Helpers/parsed_type.h"

#include "AST/ast_node.h"

#include "Lexer/tokens.h"

#include "Compiler/compiler_context.h"

// Token management
void initParser(Parser* parser, TokenBuffer* buffer, ParserMode mode, CompilerContext* ctx, bool preserveDirectives);
void advance(Parser* parser);

// Parser cloning
Parser cloneParserWithFreshLexer(Parser* original);
void freeParserClone(Parser* parser);

// Error reporting
void printParseError(const char* expected, Parser* parser);

// Token lookahead
Token peekNextToken(Parser* parser);
Token peekTwoTokensAhead(Parser* parser);
Token peekThreeTokensAhead(Parser* parser);

// Token classification helpers
const char* getOperatorString(TokenType type);
bool isAssignmentOperator(TokenType type);
bool isPreprocessorToken(TokenType type);
bool isPrimitiveTypeToken(TokenType type);
bool isModifierToken(TokenType type);
bool isStorageSpecifier(TokenType type);
bool isValidExpressionStart(TokenType type);


#endif // _PARSER_HELPERS_H
