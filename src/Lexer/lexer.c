#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>

// Lookup table for C keywords
static const struct {
    const char* word;
    TokenType type;
} keywordTable[] = {
    {"int", TOKEN_INT}, {"char", TOKEN_CHAR}, {"float", TOKEN_FLOAT}, {"double", TOKEN_DOUBLE},
    {"long", TOKEN_LONG}, {"short", TOKEN_SHORT}, {"signed", TOKEN_SIGNED}, {"unsigned", TOKEN_UNSIGNED},
    {"bool", TOKEN_BOOL}, {"enum", TOKEN_ENUM}, {"union", TOKEN_UNION}, {"struct", TOKEN_STRUCT},
    {"typedef", TOKEN_TYPEDEF}, {"void", TOKEN_VOID}, {"if", TOKEN_IF}, {"else", TOKEN_ELSE},
    {"for", TOKEN_FOR}, {"while", TOKEN_WHILE}, {"do", TOKEN_DO}, {"switch", TOKEN_SWITCH},
    {"case", TOKEN_CASE}, {"default", TOKEN_DEFAULT}, {"return", TOKEN_RETURN}, {"goto", TOKEN_GOTO},
    {"break", TOKEN_BREAK}, {"continue", TOKEN_CONTINUE}, {"extern", TOKEN_EXTERN}, {"static", TOKEN_STATIC},
    {"auto", TOKEN_AUTO}, {"register", TOKEN_REGISTER}, {"const", TOKEN_CONST}, {"volatile", TOKEN_VOLATILE},
    {"restrict", TOKEN_RESTRICT}, {"inline", TOKEN_INLINE}, 
    {"sizeof", TOKEN_SIZEOF}, {"asm", TOKEN_ASM}, 
    {NULL, TOKEN_IDENTIFIER}
};
           

void initLexer(Lexer* lexer, const char* source){
	lexer->source = source;
	lexer->position = 0;
	lexer->line = 1;
}

Token getNextToken(Lexer* lexer) {
    skipWhitespace(lexer);
    printf("DEBUG: Current char in getNextToken(): '%c' (ASCII: %d) at line %d\n", 
           lexer->source[lexer->position], lexer->source[lexer->position], lexer->line);

    if (isEOF(lexer)) {
        return (Token){TOKEN_EOF, "EOF", lexer->line};
    }

    if (isalpha(lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
        return handleIdentifierOrKeyword(lexer);
    }

    if (isdigit(lexer->source[lexer->position])) {
        return handleNumber(lexer);
    }

    if (lexer->source[lexer->position] == '"') {
        return handleStringLiteral(lexer);
    }


    if (lexer->source[lexer->position] == '\'') {
	printf("it worked it worked\n");
        return handleCharLiteral(lexer);
    }

    if (lexer->source[lexer->position] == '#') {
        return handlePreprocessorDirective(lexer);
    }

    if (lexer->source[lexer->position] == '/') {
        return handleComment(lexer);
    }

    if (strchr("=+-*/%<>!&|^~", lexer->source[lexer->position])) {
        return handleOperator(lexer);
    }

    if (strchr("?:;,(){}[].", lexer->source[lexer->position])) {
        return handlePunctuation(lexer);
    }

    return handleUnknownToken(lexer);
}


// Skips whitespace and keeps track of line numbers
void skipWhitespace(Lexer* lexer) {
    while (isspace(lexer->source[lexer->position])) {
        if (lexer->source[lexer->position] == '\n') {
            lexer->line++;
        }
        lexer->position++;
    }

}

// Checks for end of file
int isEOF(Lexer* lexer) {
    return lexer->source[lexer->position] == '\0';
}

Token handleIdentifierOrKeyword(Lexer* lexer) {
    int start = lexer->position;

    // Consume all alphanumeric characters and underscores
    while (isalnum(lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
        lexer->position++;
    }

    char* text = strndup(lexer->source + start, lexer->position - start);

    printf("DEBUG: Identified potential identifier or keyword: %s\n", text);

    // Check if the text matches a keyword
    for (int i = 0; keywordTable[i].word != NULL; i++) {
        if (strcmp(text, keywordTable[i].word) == 0) {
            printf("DEBUG: Matched keyword: %s\n", text);
            return (Token){keywordTable[i].type, text, lexer->line};
        }
    }

    printf("DEBUG: Classified as identifier: %s\n", text);
    return (Token){TOKEN_IDENTIFIER, text, lexer->line};
}


// Processes numbers (integers, floats, hex, binary, octal)
Token handleNumber(Lexer* lexer) {
    int start = lexer->position;
    TokenType type = TOKEN_NUMBER;

    // Handle hexadecimal (0x...), binary (0b...), and octal (0...)
    if (lexer->source[start] == '0') {
        if (lexer->source[start + 1] == 'x' || lexer->source[start + 1] == 'X') {
            lexer->position += 2;
            while (isxdigit(lexer->source[lexer->position])) lexer->position++;
            type = TOKEN_NUMBER; // Hex integer
        } else if (lexer->source[start + 1] == 'b' || lexer->source[start + 1] == 'B') {
            lexer->position += 2;
            while (lexer->source[lexer->position] == '0' || lexer->source[lexer->position] == '1') lexer->position++;
            type = TOKEN_NUMBER; // Binary integer
        } else {
            while (isdigit(lexer->source[lexer->position])) lexer->position++;
            type = TOKEN_NUMBER; // Octal or decimal integer
        }
    } else {
        while (isdigit(lexer->source[lexer->position])) lexer->position++;
    }

    // Handle floating-point numbers (decimal point)
    if (lexer->source[lexer->position] == '.') {
        lexer->position++;
        while (isdigit(lexer->source[lexer->position])) lexer->position++;
        type = TOKEN_FLOAT_LITERAL;
    }

    // Handle numeric suffixes (e.g., `u`, `l`, `f`)
    if (tolower(lexer->source[lexer->position]) == 'u' ||
        tolower(lexer->source[lexer->position]) == 'l' ||
        tolower(lexer->source[lexer->position]) == 'f') {
        lexer->position++;
    }

    return (Token){type, strndup(lexer->source + start, lexer->position - start), lexer->line};

}

// Processes string literals
Token handleStringLiteral(Lexer* lexer) {
    lexer->position++; // Skip opening quote
    int start = lexer->position;

    while (lexer->source[lexer->position] != '"' || lexer->source[lexer->position - 1] == '\\') {
        if (lexer->source[lexer->position] == '\0' || lexer->source[lexer->position] == '\n') {
            return (Token){TOKEN_UNKNOWN, "Unterminated string", lexer->line};
        }
        lexer->position++;
    }

    char* text = strndup(lexer->source + start, lexer->position - start);
    lexer->position++; // Consume closing quote

    return (Token){TOKEN_STRING, text, lexer->line};
}


Token handleCharLiteral(Lexer* lexer) {
    printf("DEBUG: Entering handleCharLiteral() at line %d\n", lexer->line);
    lexer->position++; // Skip opening single quote
    char current = lexer->source[lexer->position];

    // Handle escape sequences
    if (current == '\\') {
        lexer->position++;
        switch (lexer->source[lexer->position]) {
            case 'n': current = '\n'; break;
            case 't': current = '\t'; break;
            case 'r': current = '\r'; break;
            case 'b': current = '\b'; break;
            case 'f': current = '\f'; break;
            case 'a': current = '\a'; break;
            case 'v': current = '\v'; break;
            case '\\': current = '\\'; break;
            case '\'': current = '\''; break;
            case '\"': current = '\"'; break;
            default: return (Token){TOKEN_UNKNOWN, "Invalid escape sequence", lexer->line};
        }
    }

    lexer->position++;

    // Ensure valid single-character literal
    if (lexer->source[lexer->position] != '\'') {
        return (Token){TOKEN_UNKNOWN, "Invalid character literal", lexer->line};
    }
    lexer->position++; // Consume closing single quote

    char text[4] = {'\'', current, '\'', '\0'};  // Store full character literal with quotes

    printf("DEBUG: Created TOKEN_CHAR_LITERAL with value %s at line %d\n", text, lexer->line);
    
    return (Token){TOKEN_CHAR_LITERAL, strdup(text), lexer->line};
}

// Processes specific preprocessor directives (#define, #include, etc.)
Token handlePreprocessorDirective(Lexer* lexer) {
    int start = lexer->position;
    lexer->position++; // Consume '#'

    // Skip whitespace after '#'
    while (isspace(lexer->source[lexer->position])) {
        if (lexer->source[lexer->position] == '\n') lexer->line++;
        lexer->position++;
    }

    // Capture directive keyword (e.g., include, define)
    int directiveStart = lexer->position;
    while (isalnum(lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
        lexer->position++;
    }

    char* directive = strndup(lexer->source + directiveStart, lexer->position - directiveStart);

    // Match known directives
    if (strcmp(directive, "include") == 0) {
        return (Token){TOKEN_INCLUDE, directive, lexer->line};
    } else if (strcmp(directive, "define") == 0) {
        return (Token){TOKEN_DEFINE, directive, lexer->line};
    } else if (strcmp(directive, "ifdef") == 0) {
        return (Token){TOKEN_IFDEF, directive, lexer->line};
    } else if (strcmp(directive, "ifndef") == 0) {
        return (Token){TOKEN_IFNDEF, directive, lexer->line};
    } else if (strcmp(directive, "endif") == 0) {
        return (Token){TOKEN_ENDIF, directive, lexer->line};
    }

    // Fallback: unknown preprocessor directive
    return (Token){TOKEN_PREPROCESSOR_OTHER, directive, lexer->line};
}


// Processes comments (single-line `//` and multi-line `/* */`)
Token handleComment(Lexer* lexer) {
    printf("DEBUG: Entering handleComment() at line %d, current char '%c'\n",
           lexer->line, lexer->source[lexer->position]);
    int start = lexer->position - 1; // Start from '/'

    lexer->position++;
    
    if (lexer->source[lexer->position] == '=') { // Handle `/=`
        lexer->position++;
        return (Token){TOKEN_DIV_ASSIGN, "/=", lexer->line};
    }

    //  **Skip Single-line comments (`// ...`)**
    if (lexer->source[lexer->position] == '/') {
        lexer->position++;
        while (lexer->source[lexer->position] != '\n' && lexer->source[lexer->position] != '\0') {
            lexer->position++;
        }
        printf("DEBUG: Skipped single-line comment at line %d\n", lexer->line);
        return getNextToken(lexer);  //  **Skip the comment and return the next real token**
    }

    //  **Skip Multi-line comments (`/* ... */`)**
    if (lexer->source[lexer->position] == '*') {
        lexer->position++;
        while (!(lexer->source[lexer->position] == '*' && lexer->source[lexer->position + 1] == '/')) {
            if (lexer->source[lexer->position] == '\0') {
                return (Token){TOKEN_UNKNOWN, "Unterminated comment", lexer->line};
            }
            if (lexer->source[lexer->position] == '\n') {
                lexer->line++; // Handle new lines in multi-line comments
            }
            lexer->position++;
        }
        lexer->position += 2; // Skip `*/`
        printf("DEBUG: Skipped multi-line comment at line %d\n", lexer->line);
        return getNextToken(lexer);  //  **Skip the comment and return the next real token**
    }

    return (Token){TOKEN_DIVIDE, strndup("/", 1), lexer->line}; // Just a single '/'
}

// Processes all operator tokens (arithmetic, logical, bitwise, assignments)
Token handleOperator(Lexer* lexer) {
    char current = lexer->source[lexer->position];
    lexer->position++; // Move past the current character

    switch (current) {
        case '=':
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return (Token){TOKEN_EQUAL, "==", lexer->line};
            }
            return (Token){TOKEN_ASSIGN, "=", lexer->line};

        case '+':
            if (lexer->source[lexer->position] == '+') {
                lexer->position++;
                return (Token){TOKEN_INCREMENT, "++", lexer->line};
            }
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return (Token){TOKEN_PLUS_ASSIGN, "+=", lexer->line};
            }
            return (Token){TOKEN_PLUS, "+", lexer->line};

        case '-':
            if (lexer->source[lexer->position] == '-') {
                lexer->position++;
                return (Token){TOKEN_DECREMENT, "--", lexer->line};
            }
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return (Token){TOKEN_MINUS_ASSIGN, "-=", lexer->line};  // Fixed!
            }
	    if (lexer->source[lexer->position] == '>') {
                lexer->position++;
                return (Token){TOKEN_ARROW, "->", lexer->line};  // Fixed!
            }
            return (Token){TOKEN_MINUS, "-", lexer->line};
	case '*':
            //  Check if it's an assignment (`*=`)
            if (lexer->source[lexer->position] == '=') {
                lexer->position++; 
                return (Token){TOKEN_MULT_ASSIGN, "*=", lexer->line};
            }

            //  Otherwise, always return `*` as TOKEN_ASTERISK
            return (Token){TOKEN_ASTERISK, "*", lexer->line};

        case '/':
            return handleComment(lexer); // Redirects to comment handler

        case '%':
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return (Token){TOKEN_MOD_ASSIGN, "%=", lexer->line};
            }
            return (Token){TOKEN_MODULO, "%", lexer->line};

        case '<':
            if (lexer->source[lexer->position] == '<') {
                lexer->position++;
                return (Token){TOKEN_LEFT_SHIFT, "<<", lexer->line};
            }
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return (Token){TOKEN_LESS_EQUAL, "<=", lexer->line};
            }
            return (Token){TOKEN_LESS, "<", lexer->line};

        case '>':
            if (lexer->source[lexer->position] == '>') {
                lexer->position++;
                return (Token){TOKEN_RIGHT_SHIFT, ">>", lexer->line};
            }
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return (Token){TOKEN_GREATER_EQUAL, ">=", lexer->line};
            }
            return (Token){TOKEN_GREATER, ">", lexer->line};

        case '!':
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return (Token){TOKEN_NOT_EQUAL, "!=", lexer->line};
            }
            return (Token){TOKEN_LOGICAL_NOT, "!", lexer->line};

        case '&':
            if (lexer->source[lexer->position] == '&') {
                lexer->position++;
                return (Token){TOKEN_LOGICAL_AND, "&&", lexer->line};
            }
            return (Token){TOKEN_BITWISE_AND, "&", lexer->line};

        case '|':
            if (lexer->source[lexer->position] == '|') {
                lexer->position++;
                return (Token){TOKEN_LOGICAL_OR, "||", lexer->line};
            }
            return (Token){TOKEN_BITWISE_OR, "|", lexer->line};

	case '^':
	    if (lexer->source[lexer->position] == '=') {
	        lexer->position++;
	        return (Token){TOKEN_BITWISE_XOR_ASSIGN, "^=", lexer->line};
	    }
	    return (Token){TOKEN_BITWISE_XOR, "^", lexer->line};

	// Call helper methods for single-character operators and punctuation
        case '~': return (Token){TOKEN_BITWISE_NOT, "~", lexer->line};
        case ';': return handlePunctuation(lexer);
        case ',': return handlePunctuation(lexer);
        case '.': return handlePunctuation(lexer);
        case '(': return handlePunctuation(lexer);
        case ')': return handlePunctuation(lexer);
        case '{': return handlePunctuation(lexer);
        case '}': return handlePunctuation(lexer);
        case '[': return handlePunctuation(lexer);
        case ']': return handlePunctuation(lexer);

        default:
            return handleUnknownToken(lexer); // Handle any unknown characters
    }
}

// Processes punctuation and delimiters (brackets, parentheses, semicolons, etc.)
Token handlePunctuation(Lexer* lexer) {
    char current = lexer->source[lexer->position];
    lexer->position++; // Move past the punctuation character

    switch (current) {
	case '?': return (Token){TOKEN_QUESTION, "?", lexer->line};
        case ';': return (Token){TOKEN_SEMICOLON, ";", lexer->line};
        case ',': return (Token){TOKEN_COMMA, ",", lexer->line};
        case '.': return (Token){TOKEN_DOT, ".", lexer->line};
        case '(': return (Token){TOKEN_LPAREN, "(", lexer->line};
        case ')': return (Token){TOKEN_RPAREN, ")", lexer->line};
        case '{': return (Token){TOKEN_LBRACE, "{", lexer->line};
        case '}': return (Token){TOKEN_RBRACE, "}", lexer->line};
        case '[': return (Token){TOKEN_LBRACKET, "[", lexer->line};
        case ']': return (Token){TOKEN_RBRACKET, "]", lexer->line};
        case ':': return (Token){TOKEN_COLON, ":", lexer->line}; // Added support for switch-case
        default:
            printf("Warning: Unknown punctuation '%c' at line %d\n", current, lexer->line);
            return (Token){TOKEN_UNKNOWN, "Unknown punctuation", lexer->line};
    }
}


// Handles unknown tokens (invalid characters, etc.)
Token handleUnknownToken(Lexer* lexer) {
    char current = lexer->source[lexer->position];
    lexer->position++; // Move past the current character
    return (Token){TOKEN_UNKNOWN, "ERROR", lexer->line};
}

