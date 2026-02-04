#include "Parser/Helpers/parser_helpers.h"
#include "Parser/Helpers/parser_lookahead.h"
#include "parser_config.h"
#include "Compiler/diagnostics.h"
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy

// Initialize parser with a fresh token window
void initParser(Parser* parser, TokenBuffer* buffer, ParserMode mode, CompilerContext* ctx, bool preserveDirectives) {
    parser->tokenBuffer = buffer;
    parser->cursor = 0;
    parser->mode = mode;
    parser->currentToken      = *token_buffer_peek(buffer, 0);
    parser->nextToken         = *token_buffer_peek(buffer, 1);
    parser->nextNextToken     = *token_buffer_peek(buffer, 2);
    parser->nextNextNextToken = *token_buffer_peek(buffer, 3);
    parser->ctx = ctx;
    parser->preserveDirectives = preserveDirectives;
    parser->suppressErrors = false;
    const char* gnuEnv = getenv("ENABLE_GNU_STATEMENT_EXPRESSIONS");
    parser->enableStatementExpressions =
        (gnuEnv && gnuEnv[0] != '\0' && gnuEnv[0] != '0');
}

// Advance the 4-token lookahead window forward
void advance(Parser* parser) {
    if (!parser || !parser->tokenBuffer) return;
    parser->cursor++;
    parser->currentToken      = *token_buffer_peek(parser->tokenBuffer, parser->cursor);
    parser->nextToken         = *token_buffer_peek(parser->tokenBuffer, parser->cursor + 1);
    parser->nextNextToken     = *token_buffer_peek(parser->tokenBuffer, parser->cursor + 2);
    parser->nextNextNextToken = *token_buffer_peek(parser->tokenBuffer, parser->cursor + 3);
}

void advanceClone(Parser* p) {
    advance(p);
}

// ---------- SAFE CLONE HELPERS ----------

Parser cloneParserWithFreshLexer(Parser* original) {
    Parser clone = *original;
    // Keep context so typedef/tag lookups remain accurate during speculative parses.
    clone.suppressErrors = true;
    return clone;
}

void freeParserClone(Parser* parser) {
    (void)parser;
}



static bool isSystemHeaderPath(const char* file) {
    if (!file) return false;
    const char* kCLT = "/Library/Developer/CommandLineTools/SDKs/";
    const char* kUsr = "/usr/include";
    const char* kXcode = "/Applications/Xcode.app/Contents/Developer/";
    return (strncmp(file, kCLT, strlen(kCLT)) == 0) ||
           (strncmp(file, kUsr, strlen(kUsr)) == 0) ||
           (strncmp(file, kXcode, strlen(kXcode)) == 0);
}

void printParseError(const char* expected, Parser* parser) {
    if (!parser) return;
    if (parser->suppressErrors) return;
    const Token* tok = &parser->currentToken;
    const char* got = tok && tok->value ? tok->value : "<eof>";
    const char* file = tok && tok->location.start.file ? tok->location.start.file : "<unknown>";
    int line = tok ? tok->location.start.line : -1;
    if (isSystemHeaderPath(file)) {
        const char* env = getenv("FISICS_SHOW_SYSTEM_PARSE_ERRORS");
        if (!env || env[0] == '\0' || env[0] == '0') {
            return;
        }
    }
    if (parser->ctx) {
        compiler_report_diag(parser->ctx,
                             tok ? tok->location : (SourceRange){0},
                             DIAG_ERROR,
                             CDIAG_PARSER_GENERIC,
                             NULL,
                             "expected '%s', got '%s' (type %d)",
                             expected ? expected : "",
                             got,
                             tok ? tok->type : -1);
    }
    fprintf(stderr, "Error: %s at %s:%d (got '%s')\n",
            expected ? expected : "",
            file,
            line,
            got);
}




Token peekNextToken(Parser* parser) {
    PARSER_DEBUG_PRINTF("DEBUG: peekNextToken() returning %s (TokenType: %d)\n",
           parser->nextToken.value, parser->nextToken.type);
           
    return parser->nextToken;
}          
           
Token peekTwoTokensAhead(Parser* parser) {
        PARSER_DEBUG_PRINTF("DEBUG: peekTwoTokensAhead() returning %s (TokenType: %d)\n",
           parser->nextNextToken.value, parser->nextNextToken.type);
 
    return parser->nextNextToken;
}
    
Token peekThreeTokensAhead(Parser* parser) {
        PARSER_DEBUG_PRINTF("DEBUG: peekThreeTokensAhead() returning %s (TokenType: %d)\n",
           parser->nextNextNextToken.value, parser->nextNextNextToken.type);
        
    return parser->nextNextNextToken;
}   



const char* getOperatorString(TokenType type) {
    switch(type) {
        // Comparison Operators
        case TOKEN_LESS: return "<";
        case TOKEN_GREATER: return ">";
        case TOKEN_LESS_EQUAL: return "<=";
        case TOKEN_GREATER_EQUAL: return ">=";
        case TOKEN_EQUAL: return "==";
        case TOKEN_NOT_EQUAL: return "!=";
    
        // Logical Operators
        case TOKEN_LOGICAL_AND: return "&&";
        case TOKEN_LOGICAL_OR: return "||";
        case TOKEN_LOGICAL_NOT: return "!";
 
        // Bitwise Operators
        case TOKEN_BITWISE_AND: return "&";
        case TOKEN_BITWISE_OR: return "|";
        case TOKEN_BITWISE_XOR: return "^";
        case TOKEN_BITWISE_NOT: return "~";
        case TOKEN_LEFT_SHIFT: return "<<";
        case TOKEN_RIGHT_SHIFT: return ">>";
 
        // Arithmetic Operators
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_ASTERISK: return "*";
        case TOKEN_DIVIDE: return "/";
        case TOKEN_MODULO: return "%";
    
        // Assignment Operators
        case TOKEN_ASSIGN: return "=";
        case TOKEN_PLUS_ASSIGN: return "+=";
        case TOKEN_MINUS_ASSIGN: return "-=";
        case TOKEN_MULT_ASSIGN: return "*=";
        case TOKEN_DIV_ASSIGN: return "/=";
        case TOKEN_MOD_ASSIGN: return "%=";
        case TOKEN_BITWISE_AND_ASSIGN: return "&=";
        case TOKEN_BITWISE_OR_ASSIGN: return "|=";
        case TOKEN_BITWISE_XOR_ASSIGN: return "^=";
        case TOKEN_LEFT_SHIFT_ASSIGN: return "<<=";
        case TOKEN_RIGHT_SHIFT_ASSIGN: return ">>=";
        case TOKEN_INCREMENT: return "++";
        case TOKEN_DECREMENT: return "--";
        
        default: return "UNKNOWN";
    }   
}

bool isAssignmentOperator(TokenType type) {
    return type == TOKEN_ASSIGN ||
           type == TOKEN_PLUS_ASSIGN ||
           type == TOKEN_MINUS_ASSIGN ||
           type == TOKEN_MULT_ASSIGN || 
           type == TOKEN_DIV_ASSIGN ||  
           type == TOKEN_MOD_ASSIGN ||  
           type == TOKEN_BITWISE_AND_ASSIGN ||
           type == TOKEN_BITWISE_OR_ASSIGN || 
           type == TOKEN_BITWISE_XOR_ASSIGN || 
           type == TOKEN_LEFT_SHIFT_ASSIGN ||  
           type == TOKEN_RIGHT_SHIFT_ASSIGN;   
}
 
bool isPreprocessorToken(TokenType type) {
    return type == TOKEN_INCLUDE || type == TOKEN_DEFINE ||
           type == TOKEN_UNDEF ||
           type == TOKEN_IFDEF || type == TOKEN_IFNDEF ||  
           type == TOKEN_ENDIF;
}
 
bool isPrimitiveTypeToken(TokenType type) {
    return type == TOKEN_INT ||
           type == TOKEN_FLOAT ||
           type == TOKEN_DOUBLE ||
           type == TOKEN_CHAR ||
           type == TOKEN_BOOL ||
           type == TOKEN_VOID ||
           type == TOKEN_LONG ||
           type == TOKEN_SHORT ||
           type == TOKEN_SIGNED ||
           type == TOKEN_UNSIGNED;
}
 
bool isModifierToken(TokenType type) {
    return type == TOKEN_CONST ||
           type == TOKEN_VOLATILE ||
           type == TOKEN_RESTRICT ||
           type == TOKEN_INLINE;
}
 
bool isStorageSpecifier(TokenType type) {
    return type == TOKEN_STATIC || type == TOKEN_EXTERN || type == TOKEN_AUTO || type == TOKEN_REGISTER;  
}


bool isValidExpressionStart(TokenType type) {
    return
        type == TOKEN_IDENTIFIER ||
        type == TOKEN_LPAREN ||
        type == TOKEN_NUMBER ||
        type == TOKEN_FLOAT_LITERAL ||
        type == TOKEN_STRING ||
        type == TOKEN_CHAR_LITERAL ||
        type == TOKEN_MINUS ||
        type == TOKEN_PLUS ||
        type == TOKEN_LOGICAL_NOT ||
        type == TOKEN_BITWISE_NOT ||
        type == TOKEN_INCREMENT ||
        type == TOKEN_DECREMENT ||
        type == TOKEN_ASTERISK ||     // for pointer dereference
	type == TOKEN_BITWISE_AND ||   // address-of
        type == TOKEN_SIZEOF ||
        type == TOKEN_ALIGNOF;
}
