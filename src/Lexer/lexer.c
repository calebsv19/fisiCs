#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"

#include "Lexer/keyword_lookup.h"

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>

int print_statements = 0;

void initLexer(Lexer* lexer, const char* source){
	lexer->source = source;
	lexer->position = 0;
	lexer->line = 1;
}

Token getNextToken(Lexer* lexer) {
    skipWhitespace(lexer);

    if (print_statements == 1){
    	printf("DEBUG: Current char in getNextToken(): '%c' (ASCII: %d) at line %d\n", 
    	 	      lexer->source[lexer->position], lexer->source[lexer->position], lexer->line);
    }

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

const char *lookupKeyword(const char *str, size_t len) {
    return in_keyword_set(str, len);
}

Token handleIdentifierOrKeyword(Lexer* lexer) {
    int start = lexer->position;

    // Consume all alphanumeric characters and underscores
    while (isalnum(lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
        lexer->position++;
    }

    char* text = strndup(lexer->source + start, lexer->position - start);
    if (print_statements == 1){
    	printf("DEBUG: Identified potential identifier or keyword: %s\n", text);
    }

    // Use gperf string-based lookup
    const char* matchedKeyword = lookupKeyword(text, strlen(text));
    if (matchedKeyword) {
        TokenType tokenType = keywordToTokenType(matchedKeyword);
        
	if (print_statements == 1){        
		printf("DEBUG: Matched keyword: %s → TokenType: %d\n", text, tokenType);
        }
        return (Token){tokenType, text, lexer->line};
    }

    if (print_statements == 1){
    	printf("DEBUG: Classified as identifier: %s\n", text);
    }

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
    lexer->position++; // skip opening '

    int val = 0;
    char ch = lexer->source[lexer->position++];

    if (ch == '\\') {
        char e = lexer->source[lexer->position++];
        switch (e) {
            case 'n':  val = '\n'; break;
            case 't':  val = '\t'; break;
            case 'r':  val = '\r'; break;
            case 'b':  val = '\b'; break;
            case 'f':  val = '\f'; break;
            case 'a':  val = '\a'; break;
            case 'v':  val = '\v'; break;
            case '\\': val = '\\'; break;
            case '\'': val = '\''; break;
            case '\"': val = '\"'; break;

            // \xHH… (1+ hex digits)
            case 'x': {
                int hex = 0, any = 0;
                while (isxdigit((unsigned char)lexer->source[lexer->position])) {
                    char h = lexer->source[lexer->position++];
                    hex *= 16;
                    if (h >= '0' && h <= '9') hex += (h - '0');
                    else if (h >= 'a' && h <= 'f') hex += (h - 'a' + 10);
                    else hex += (h - 'A' + 10);
                    any = 1;
                }
                if (!any) return (Token){TOKEN_UNKNOWN, "Invalid \\x escape", lexer->line};
                val = hex & 0xFF;
                break;
            }

            // Octal \nnn (up to 3 octal digits; first already in e)
            default:
                if (e >= '0' && e <= '7') {
                    int oct = (e - '0');
                    for (int k = 0; k < 2; ++k) {
                        char d = lexer->source[lexer->position];
                        if (d < '0' || d > '7') break;
                        lexer->position++;
                        oct = (oct << 3) + (d - '0');
                    }
                    val = oct & 0xFF;
                } else {
                    // unknown escape, take literally
                    val = (unsigned char)e;
                }
                break;
        }
    } else {
        val = (unsigned char)ch;
    }

    if (lexer->source[lexer->position] != '\'')
        return (Token){TOKEN_UNKNOWN, "Invalid character literal", lexer->line};

    lexer->position++; // consume closing '

    // store numeric value as text, keeps your existing print style simple
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", val);
    printf("DEBUG: Created TOKEN_CHAR_LITERAL with value %s at line %d\n", buf, lexer->line);
    return (Token){TOKEN_CHAR_LITERAL, strdup(buf), lexer->line};
}

// Processes specific preprocessor directives (#define, #include, etc.)
Token handlePreprocessorDirective(Lexer* lexer) {
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
	    if (lexer->source[lexer->position] == '<') {                  // << or <<=
	        lexer->position++;
	        if (lexer->source[lexer->position] == '=') {              // NEW: <<=
	            lexer->position++;
	            return (Token){TOKEN_LEFT_SHIFT_ASSIGN, "<<=", lexer->line};
	        }
	        return (Token){TOKEN_LEFT_SHIFT, "<<", lexer->line};
	    }
	    if (lexer->source[lexer->position] == '=') {
	        lexer->position++;
	        return (Token){TOKEN_LESS_EQUAL, "<=", lexer->line};
	    }
	    return (Token){TOKEN_LESS, "<", lexer->line};
	
	case '>':
	    if (lexer->source[lexer->position] == '>') {                  // >> or >>=
	        lexer->position++;
	        if (lexer->source[lexer->position] == '=') {              // NEW: >>=
	            lexer->position++;
	            return (Token){TOKEN_RIGHT_SHIFT_ASSIGN, ">>=", lexer->line};
	        }
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
	    if (lexer->source[lexer->position] == '=') {                 // NEW: &=
	        lexer->position++;
	        return (Token){TOKEN_BITWISE_AND_ASSIGN, "&=", lexer->line};
	    }
	    return (Token){TOKEN_BITWISE_AND, "&", lexer->line};
	
	case '|':
	    if (lexer->source[lexer->position] == '|') {
	        lexer->position++;
	        return (Token){TOKEN_LOGICAL_OR, "||", lexer->line};
	    }
	    if (lexer->source[lexer->position] == '=') {                  // NEW: |=
	        lexer->position++;
	        return (Token){TOKEN_BITWISE_OR_ASSIGN, "|=", lexer->line};
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
    lexer->position++; // Move past the current character
    return (Token){TOKEN_UNKNOWN, "ERROR", lexer->line};
}



TokenType keywordToTokenType(const char* word) {
    if (strcmp(word, "int") == 0) return TOKEN_INT;
    if (strcmp(word, "char") == 0) return TOKEN_CHAR;
    if (strcmp(word, "float") == 0) return TOKEN_FLOAT;
    if (strcmp(word, "double") == 0) return TOKEN_DOUBLE;
    if (strcmp(word, "long") == 0) return TOKEN_LONG;
    if (strcmp(word, "short") == 0) return TOKEN_SHORT;
    if (strcmp(word, "signed") == 0) return TOKEN_SIGNED;
    if (strcmp(word, "unsigned") == 0) return TOKEN_UNSIGNED;
    if (strcmp(word, "bool") == 0) return TOKEN_BOOL;
    if (strcmp(word, "true") == 0) return TOKEN_TRUE;
    if (strcmp(word, "false") == 0) return TOKEN_FALSE;
    if (strcmp(word, "enum") == 0) return TOKEN_ENUM;
    if (strcmp(word, "union") == 0) return TOKEN_UNION;
    if (strcmp(word, "struct") == 0) return TOKEN_STRUCT;
    if (strcmp(word, "typedef") == 0) return TOKEN_TYPEDEF;
    if (strcmp(word, "void") == 0) return TOKEN_VOID;
    if (strcmp(word, "if") == 0) return TOKEN_IF;
    if (strcmp(word, "else") == 0) return TOKEN_ELSE;
    if (strcmp(word, "for") == 0) return TOKEN_FOR;
    if (strcmp(word, "while") == 0) return TOKEN_WHILE;
    if (strcmp(word, "do") == 0) return TOKEN_DO;
    if (strcmp(word, "switch") == 0) return TOKEN_SWITCH;
    if (strcmp(word, "case") == 0) return TOKEN_CASE;
    if (strcmp(word, "default") == 0) return TOKEN_DEFAULT;
    if (strcmp(word, "return") == 0) return TOKEN_RETURN;
    if (strcmp(word, "goto") == 0) return TOKEN_GOTO;
    if (strcmp(word, "break") == 0) return TOKEN_BREAK;
    if (strcmp(word, "continue") == 0) return TOKEN_CONTINUE;
    if (strcmp(word, "extern") == 0) return TOKEN_EXTERN;
    if (strcmp(word, "static") == 0) return TOKEN_STATIC;
    if (strcmp(word, "auto") == 0) return TOKEN_AUTO;
    if (strcmp(word, "register") == 0) return TOKEN_REGISTER;
    if (strcmp(word, "const") == 0) return TOKEN_CONST;
    if (strcmp(word, "volatile") == 0) return TOKEN_VOLATILE;
    if (strcmp(word, "restrict") == 0) return TOKEN_RESTRICT;
    if (strcmp(word, "inline") == 0) return TOKEN_INLINE;
    if (strcmp(word, "sizeof") == 0) return TOKEN_SIZEOF;
    if (strcmp(word, "asm") == 0) return TOKEN_ASM;
    if (strcmp(word, "pragma") == 0) return TOKEN_PRAGMA;
    if (strcmp(word, "once") == 0) return TOKEN_ONCE;
    return TOKEN_IDENTIFIER;
}
