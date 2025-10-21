#ifndef _PARSER_HELPERS_H
#define _PARSER_HELPERS_H

#include "parser.h"
#include "Parser/Helpers/parsed_type.h"

#include "AST/ast_node.h"

#include "Lexer/tokens.h"
#include "Lexer/lexer.h"

#include "Compiler/compiler_context.h"

// Token management
void initParser(Parser* parser, Lexer* lexer, ParserMode mode, CompilerContext* ctx);
void advance(Parser* parser);

// Lexer cloning
Lexer* cloneLexer(const Lexer* original);
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
bool isKnownType(const char* name);
bool isValidExpressionStart(TokenType type);


#endif // _PARSER_HELPERS_H

