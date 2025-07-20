#include "parser_helpers.h"
#include "parser_lookahead.h"
#include "parser_config.h"


// Initialize parser with a fresh token window
void initParser(Parser* parser, Lexer* lexer, ParserMode mode) {
    parser->lexer = lexer;
    parser->mode = mode;
    parser->currentToken = getNextToken(lexer);
    parser->nextToken = getNextToken(lexer);
    parser->nextNextToken = getNextToken(lexer);
    parser->nextNextNextToken = getNextToken(lexer);
}

// Advance the 4-token lookahead window forward
void advance(Parser* parser) {
    parser->currentToken = parser->nextToken;
    parser->nextToken = parser->nextNextToken;
    parser->nextNextToken = parser->nextNextNextToken;
    parser->nextNextNextToken = getNextToken(parser->lexer);
}

void advanceClone(Parser* p) {
    p->currentToken = p->nextToken;
    p->nextToken = p->nextNextToken;
    p->nextNextToken = p->nextNextNextToken;
    p->nextNextNextToken = getNextToken(p->lexer);
}



// Clone a lexer safely for use in simulated lookahead
Lexer* cloneLexer(const Lexer* original) {
    Lexer* copy = malloc(sizeof(Lexer));
    if (!copy) return NULL;
    memcpy(copy, original, sizeof(Lexer));  // More future-proof
    return copy;
}


Parser cloneParserWithFreshLexer(Parser* original) {
    Parser temp = *original;


    // Clone the lexer, but reset to same position
    temp.lexer = cloneLexer(original->lexer);

    // Copy token window manually instead of re-tokenizing
    temp.currentToken = original->currentToken;
    temp.nextToken = original->nextToken;
    temp.nextNextToken = original->nextNextToken;
    temp.nextNextNextToken = original->nextNextNextToken;

    return temp;
}


// Free the cloned lexer when done simulating lookahead
void freeParserClone(Parser* parser) {
    if (parser->lexer) {
        free(parser->lexer);
        parser->lexer = NULL;
    }
}




void printParseError(const char* expected, Parser* parser) {
    printf("Error: expected '%s' at line %d, got '%s' (Type: %d)\n",
           expected, parser->currentToken.line, parser->currentToken.value, 
			parser->currentToken.type);
}




Token peekNextToken(Parser* parser) {
    printf("DEBUG: peekNextToken() returning %s (TokenType: %d)\n",
           parser->nextToken.value, parser->nextToken.type);
           
    return parser->nextToken;
}          
           
Token peekTwoTokensAhead(Parser* parser) {
        printf("DEBUG: peekTwoTokensAhead() returning %s (TokenType: %d)\n",
           parser->nextNextToken.value, parser->nextNextToken.type);
 
    return parser->nextNextToken;
}
    
Token peekThreeTokensAhead(Parser* parser) {
        printf("DEBUG: peekThreeTokensAhead() returning %s (TokenType: %d)\n",
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
           type == TOKEN_IFDEF || type == TOKEN_IFNDEF ||  
           type == TOKEN_ENDIF;
}
 
bool isPrimitiveTypeToken(TokenType type) {
    return type == TOKEN_INT || type == TOKEN_FLOAT || type == TOKEN_DOUBLE ||
                type == TOKEN_CHAR || type == TOKEN_BOOL || type == TOKEN_VOID;
}
 
bool isModifierToken(TokenType type) {
    return type == TOKEN_CONST || type == TOKEN_UNSIGNED ||
           type == TOKEN_SIGNED || type == TOKEN_SHORT || type == TOKEN_LONG ||
           type == TOKEN_VOLATILE || type == TOKEN_RESTRICT || type == TOKEN_INLINE;
}
 
bool isStorageSpecifier(TokenType type) {
    return type == TOKEN_STATIC || type == TOKEN_EXTERN || type == TOKEN_AUTO || type == TOKEN_REGISTER;  
}


bool isKnownType(const char* name) {
    return strcmp(name, "int") == 0 ||
           strcmp(name, "float") == 0 ||
           strcmp(name, "char") == 0 ||
           strcmp(name, "double") == 0 ||
           strcmp(name, "bool") == 0 ||
           strcmp(name, "void") == 0;
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
        type == TOKEN_SIZEOF;
}
