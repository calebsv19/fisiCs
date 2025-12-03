#ifndef TOKENS_H
#define TOKENS_H

typedef struct {
    const char* file;
    int line;
    int column;
} SourceLocation;

typedef struct {
    SourceLocation start;
    SourceLocation end;
} SourceRange;

typedef enum {
    // ==============================
    //  Keywords (Reserved Words)
    // ==============================
    // Data Types
    TOKEN_INT, TOKEN_FLOAT, TOKEN_CHAR, TOKEN_DOUBLE, TOKEN_LONG, TOKEN_SHORT, TOKEN_SIGNED, TOKEN_UNSIGNED, 
    TOKEN_VOID, TOKEN_BOOL, TOKEN_ENUM, TOKEN_UNION, TOKEN_STRUCT, TOKEN_TYPEDEF, 
    
    // Control Flow
    TOKEN_IF, TOKEN_ELSE, 
    TOKEN_WHILE, TOKEN_FOR, TOKEN_DO, 
    TOKEN_SWITCH, TOKEN_CASE, TOKEN_DEFAULT,
    
    // Function & Returns
    TOKEN_RETURN, TOKEN_GOTO,
    
    // Loop Control
    TOKEN_BREAK, TOKEN_CONTINUE,

    // Storage Classes
    TOKEN_EXTERN, TOKEN_STATIC, TOKEN_AUTO, TOKEN_REGISTER, 
    
    // Modifiers
    TOKEN_CONST, TOKEN_VOLATILE, TOKEN_RESTRICT, TOKEN_INLINE, 
    
    // Special Values
    TOKEN_NULL, TOKEN_SIZEOF, 

    // ==============================
    //  Identifiers & Literals
    // ==============================
    TOKEN_IDENTIFIER,        // Variable & function names
    TOKEN_INCLUDE, TOKEN_DEFINE, TOKEN_UNDEF,		// Basic preprocessor
    TOKEN_IFDEF, TOKEN_IFNDEF, TOKEN_ENDIF,     // if related processor
    TOKEN_PRAGMA, TOKEN_ONCE,			// pragma handling
    TOKEN_PREPROCESSOR_OTHER,			// handles edge cases
    TOKEN_PP_IF, TOKEN_PP_ELIF, TOKEN_PP_ELSE,

    TOKEN_NUMBER, TOKEN_FLOAT_LITERAL, // Numeric literals
    TOKEN_STRING, TOKEN_CHAR_LITERAL,  // Text literals
    TOKEN_TRUE, TOKEN_FALSE,          // Boolean literals

    // ==============================
    //  Operators
    // ==============================
    
    // Assignment Operators
    TOKEN_ASSIGN,  // =
    TOKEN_PLUS_ASSIGN,  TOKEN_MINUS_ASSIGN, // += -=
    TOKEN_MULT_ASSIGN,  TOKEN_DIV_ASSIGN,   // *= /=
    TOKEN_MOD_ASSIGN,                        // %=

    // Arithmetic Operators
    TOKEN_PLUS, TOKEN_MINUS,         // + -
    TOKEN_ASTERISK, TOKEN_DIVIDE,    // * /
    TOKEN_MODULO,                    // %

    // Increment/Decrement Operators
    TOKEN_INCREMENT, TOKEN_DECREMENT, // ++ --

    // Comparison Operators
    TOKEN_EQUAL, TOKEN_NOT_EQUAL,     // == !=
    TOKEN_LESS, TOKEN_LESS_EQUAL,     // < <=
    TOKEN_GREATER, TOKEN_GREATER_EQUAL, // > >=

    // Logical Operators
    TOKEN_LOGICAL_AND, TOKEN_LOGICAL_OR, TOKEN_LOGICAL_NOT, // && || !

    // Bitwise Operators
    TOKEN_BITWISE_AND, TOKEN_BITWISE_OR, TOKEN_BITWISE_XOR, TOKEN_BITWISE_NOT, // & | ^ ~
    TOKEN_LEFT_SHIFT, TOKEN_RIGHT_SHIFT,  // << >>

    TOKEN_BITWISE_AND_ASSIGN, TOKEN_BITWISE_OR_ASSIGN, TOKEN_BITWISE_XOR_ASSIGN,  // &=, |=, ^=
    TOKEN_LEFT_SHIFT_ASSIGN, TOKEN_RIGHT_SHIFT_ASSIGN,  // <<=, >>=

    // ==============================
    //  Delimiters & Punctuation
    // ==============================
    TOKEN_SEMICOLON, TOKEN_COLON, TOKEN_COMMA, TOKEN_DOT, TOKEN_ELLIPSIS, // ; , . ...
    TOKEN_QUESTION,			     // ?
    TOKEN_HASH, TOKEN_DOUBLE_HASH,         // # ##
    TOKEN_LPAREN, TOKEN_RPAREN,              // ( )
    TOKEN_LBRACE, TOKEN_RBRACE,              // { }
    TOKEN_LBRACKET, TOKEN_RBRACKET,          // [ ]
    
    TOKEN_ARROW,	     // *(label), ()->()

    // ==============================
    //  Comments
    // ==============================
    TOKEN_LINE_COMMENT, TOKEN_BLOCK_COMMENT, // // /* */

    // ==============================
    //  Special Tokens
    // ==============================
    TOKEN_ASM,
    TOKEN_EOF,   
    TOKEN_UNKNOWN

} TokenType;


typedef struct{
    TokenType type;
    char* value;
    int line;
    SourceRange location;
    SourceRange macroCallSite;
    SourceRange macroDefinition;
} Token;

#endif
