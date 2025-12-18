#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include "lexer.h"

#include "Lexer/keyword_lookup.h"

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>

int print_statements = 0;

static int lexer_debug_flag = -1;
static int lexer_debug_enabled(void) {
    if (lexer_debug_flag < 0) {
        const char* env = getenv("FISICS_DEBUG_LEXER");
        lexer_debug_flag = (env && env[0]) ? 1 : 0;
    }
    return lexer_debug_flag;
}

#define LEXER_DEBUG_PRINTF(...) do { if (lexer_debug_enabled()) fprintf(stderr, __VA_ARGS__); } while (0)

typedef struct {
    int position;
    int line;
    int lineStart;
} LexerMark;

static inline const char* lexer_file_path(const Lexer* lexer) {
    return (lexer && lexer->filePath) ? lexer->filePath : "<unknown>";
}

static inline int lexer_compute_column(int position, int lineStart) {
    int column = (position - lineStart) + 1;
    return (column < 1) ? 1 : column;
}

static inline LexerMark lexer_mark(const Lexer* lexer) {
    LexerMark mark = {0};
    if (lexer) {
        mark.position = lexer->position;
        mark.line = lexer->line;
        mark.lineStart = lexer->lineStart;
    }
    return mark;
}

static inline LexerMark lexer_mark_previous(const Lexer* lexer) {
    LexerMark mark = lexer_mark(lexer);
    mark.position -= 1;
    return mark;
}

static inline SourceLocation lexer_build_location(const Lexer* lexer,
                                                  int position,
                                                  int line,
                                                  int lineStart) {
    SourceLocation loc;
    loc.file = lexer_file_path(lexer);
    loc.line = line;
    loc.column = lexer_compute_column(position, lineStart);
    return loc;
}

static inline SourceRange lexer_build_range(const Lexer* lexer, LexerMark start) {
    SourceRange range;
    range.start = lexer_build_location(lexer, start.position, start.line, start.lineStart);
    range.end   = lexer_build_location(lexer, lexer ? lexer->position : 0,
                                       lexer ? lexer->line : 0,
                                       lexer ? lexer->lineStart : 0);
    return range;
}

static inline SourceRange empty_source_range(void) {
    SourceRange range;
    range.start.file = NULL;
    range.start.line = 0;
    range.start.column = 0;
    range.end = range.start;
    return range;
}

static inline Token make_token(Lexer* lexer, TokenType type, char* value, LexerMark start) {
    Token token;
    token.type = type;
    token.value = value;
    token.line = start.line;
    token.location = lexer_build_range(lexer, start);
    token.macroCallSite = empty_source_range();
    token.macroDefinition = empty_source_range();
    return token;
}

static char translate_trigraph_char(char c) {
    switch (c) {
        case '=': return '#';
        case '/': return '\\';
        case '\'': return '^';
        case '<': return '{';
        case '>': return '}';
        case '!': return '|';
        case '(': return '[';
        case ')': return ']';
        case '-': return '~';
        default: return 0;
    }
}

static char* translate_source(const char* source, bool enableTrigraphs, bool* outSawTrigraph) {
    if (!source) return NULL;
    size_t len = strlen(source);
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    size_t w = 0;
    bool sawTrigraph = false;
    for (size_t i = 0; i < len; ) {
        if (enableTrigraphs && i + 2 < len && source[i] == '?' && source[i + 1] == '?') {
            char mapped = translate_trigraph_char(source[i + 2]);
            if (mapped) {
                out[w++] = mapped;
                i += 3;
                sawTrigraph = true;
                continue;
            }
        }
        if (i + 3 < len && source[i] == '%' && source[i + 1] == ':' &&
            source[i + 2] == '%' && source[i + 3] == ':') {
            out[w++] = '#';
            out[w++] = '#';
            i += 4;
            continue;
        }
        if (i + 1 < len && source[i] == '%' && source[i + 1] == ':') {
            out[w++] = '#';
            i += 2;
            continue;
        }
        if (i + 1 < len && source[i] == '<' && source[i + 1] == ':') {
            out[w++] = '[';
            i += 2;
            continue;
        }
        if (i + 1 < len && source[i] == ':' && source[i + 1] == '>') {
            out[w++] = ']';
            i += 2;
            continue;
        }
        if (i + 1 < len && source[i] == '<' && source[i + 1] == '%') {
            out[w++] = '{';
            i += 2;
            continue;
        }
        if (i + 1 < len && source[i] == '%' && source[i + 1] == '>') {
            out[w++] = '}';
            i += 2;
            continue;
        }
        out[w++] = source[i++];
    }
    out[w] = '\0';
    if (outSawTrigraph) *outSawTrigraph = sawTrigraph;
    char* shrunk = realloc(out, w + 1);
    return shrunk ? shrunk : out;
}

void initLexer(Lexer* lexer, const char* source, const char* filePath, bool enableTrigraphs){
	lexer->ownedSource = NULL;
	lexer->enableTrigraphs = enableTrigraphs;
	lexer->filePath = filePath;
	lexer->position = 0;
    lexer->length = 0;
	lexer->line = 1;
	lexer->lineStart = 0;
    bool sawTrigraph = false;
    const char* rawSource = source ? source : "";
    char* translated = translate_source(rawSource, enableTrigraphs, &sawTrigraph);
    if (translated) {
        lexer->ownedSource = translated;
        lexer->source = translated;
    } else {
        lexer->source = rawSource;
    }
    lexer->length = (int)strlen(lexer->source);
    if (sawTrigraph && !enableTrigraphs) {
        fprintf(stderr, "warning: trigraphs present; enable trigraph translation to honor them\n");
    }
}

void destroyLexer(Lexer* lexer) {
    if (!lexer) return;
    free(lexer->ownedSource);
    lexer->ownedSource = NULL;
    lexer->source = NULL;
    lexer->length = 0;
}

Token getNextToken(Lexer* lexer) {
    skipWhitespace(lexer);

    if (lexer_debug_enabled()) {
    	LEXER_DEBUG_PRINTF("DEBUG: Current char in getNextToken(): '%c' (ASCII: %d) at line %d\n",
    	 	      (lexer->position < lexer->length ? lexer->source[lexer->position] : '\0'),
                  (lexer->position < lexer->length ? lexer->source[lexer->position] : 0),
                  lexer->line);
    }

    if (isEOF(lexer)) {
        LexerMark start = lexer_mark(lexer);
        lexer->position++; // move past terminator to avoid repeated EOF
        return make_token(lexer, TOKEN_EOF, (char*)"EOF", start);
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
        LEXER_DEBUG_PRINTF("it worked it worked\n");
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
    while (lexer->position < lexer->length) {
        char c = lexer->source[lexer->position];
        char next = (lexer->position + 1 < lexer->length) ? lexer->source[lexer->position + 1] : '\0';
        char next2 = (lexer->position + 2 < lexer->length) ? lexer->source[lexer->position + 2] : '\0';

        // Line splices: remove backslash + newline (handles \n and \r\n).
        if (c == '\\' && next == '\n') {
            lexer->position += 2;
            continue;
        }
        if (c == '\\' && next == '\r' && next2 == '\n') {
            lexer->position += 3;
            continue;
        }

        if (!isspace((unsigned char)c)) {
            break;
        }
        if (c == '\n') {
            lexer->position++;
            lexer->line++;
            lexer->lineStart = lexer->position;
            continue;
        }
        lexer->position++;
    }
}

// Checks for end of file
int isEOF(Lexer* lexer) {
    return !lexer || lexer->position >= lexer->length || lexer->source[lexer->position] == '\0';
}

const char *lookupKeyword(const char *str, size_t len) {
    return in_keyword_set(str, len);
}

Token handleIdentifierOrKeyword(Lexer* lexer) {
    LexerMark startMark = lexer_mark(lexer);
    int startPos = lexer->position;

    // Consume all alphanumeric characters and underscores
    while (isalnum(lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
        lexer->position++;
    }

    char* text = strndup(lexer->source + startPos, lexer->position - startPos);
    if (lexer_debug_enabled()) {
    	LEXER_DEBUG_PRINTF("DEBUG: Identified potential identifier or keyword: %s\n", text);
    }

    // Use gperf string-based lookup
    const char* matchedKeyword = lookupKeyword(text, strlen(text));
    if (matchedKeyword) {
        TokenType tokenType = keywordToTokenType(matchedKeyword);
        
	if (lexer_debug_enabled()){        
		LEXER_DEBUG_PRINTF("DEBUG: Matched keyword: %s → TokenType: %d\n", text, tokenType);
        }
        return make_token(lexer, tokenType, text, startMark);
    }

    /* Manual aliases for compiler builtins / GNU spellings that are not
       listed in the gperf table. */
    if (strcmp(text, "__signed") == 0 || strcmp(text, "__signed__") == 0) {
        return make_token(lexer, TOKEN_SIGNED, text, startMark);
    }
    if (strcmp(text, "__inline") == 0 || strcmp(text, "__inline__") == 0) {
        return make_token(lexer, TOKEN_INLINE, text, startMark);
    }
    if (strcmp(text, "__const") == 0 || strcmp(text, "__const__") == 0) {
        return make_token(lexer, TOKEN_CONST, text, startMark);
    }
    if (strcmp(text, "__restrict") == 0 || strcmp(text, "__restrict__") == 0) {
        return make_token(lexer, TOKEN_RESTRICT, text, startMark);
    }
    if (strcmp(text, "__asm") == 0 || strcmp(text, "__asm__") == 0) {
        return make_token(lexer, TOKEN_ASM, text, startMark);
    }

    if (lexer_debug_enabled()){
    	LEXER_DEBUG_PRINTF("DEBUG: Classified as identifier: %s\n", text);
    }

    return make_token(lexer, TOKEN_IDENTIFIER, text, startMark);
}


// Processes numbers (integers, floats, hex, binary, octal)
Token handleNumber(Lexer* lexer) {
    LexerMark startMark = lexer_mark(lexer);
    int startPos = lexer->position;
    TokenType type = TOKEN_NUMBER;

    // Handle hexadecimal (0x...), binary (0b...), and octal (0...)
    if (lexer->source[startPos] == '0') {
        if (lexer->source[startPos + 1] == 'x' || lexer->source[startPos + 1] == 'X') {
            lexer->position += 2;
            while (isxdigit(lexer->source[lexer->position])) lexer->position++;
            type = TOKEN_NUMBER; // Hex integer
        } else if (lexer->source[startPos + 1] == 'b' || lexer->source[startPos + 1] == 'B') {
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

    return make_token(lexer, type, strndup(lexer->source + startPos, lexer->position - startPos), startMark);

}

// Processes string literals
Token handleStringLiteral(Lexer* lexer) {
    LexerMark startMark = lexer_mark(lexer);
    lexer->position++; // Skip opening quote
    int start = lexer->position;

    while (lexer->source[lexer->position] != '"' || lexer->source[lexer->position - 1] == '\\') {
        if (lexer->source[lexer->position] == '\0' || lexer->source[lexer->position] == '\n') {
            return make_token(lexer, TOKEN_UNKNOWN, (char*)"Unterminated string", startMark);
        }
        lexer->position++;
    }

    char* text = strndup(lexer->source + start, lexer->position - start);
    lexer->position++; // Consume closing quote

    return make_token(lexer, TOKEN_STRING, text, startMark);
}


Token handleCharLiteral(Lexer* lexer) {
    LexerMark startMark = lexer_mark(lexer);
    LEXER_DEBUG_PRINTF("DEBUG: Entering handleCharLiteral() at line %d\n", lexer->line);
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
                if (!any) return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid \\x escape", startMark);
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
        return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid character literal", startMark);

    lexer->position++; // consume closing '

    // store numeric value as text, keeps your existing print style simple
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", val);
    LEXER_DEBUG_PRINTF("DEBUG: Created TOKEN_CHAR_LITERAL with value %s at line %d\n", buf, lexer->line);
    return make_token(lexer, TOKEN_CHAR_LITERAL, strdup(buf), startMark);
}

// Processes specific preprocessor directives (#define, #include, etc.)
Token handlePreprocessorDirective(Lexer* lexer) {
    LexerMark start = lexer_mark(lexer);
    bool atLineStart = true;
    for (int pos = lexer->lineStart; pos < start.position; ++pos) {
        char c = lexer->source[pos];
        if (c != ' ' && c != '\t' && c != '\r') {
            atLineStart = false;
            break;
        }
    }
    lexer->position++; // Consume '#'

    // Treat non-directive '#' tokens (stringification/##) when not at line start.
    if (!atLineStart) {
        if (lexer->source[lexer->position] == '#') {
            lexer->position++;
            return make_token(lexer, TOKEN_DOUBLE_HASH, strndup("##", 2), start);
        }
        return make_token(lexer, TOKEN_HASH, strndup("#", 1), start);
    }

    // Skip whitespace after '#'
    while (isspace((unsigned char)lexer->source[lexer->position])) {
        if (lexer->source[lexer->position] == '\n') {
            lexer->position++;
            lexer->line++;
            lexer->lineStart = lexer->position;
            continue;
        }
        lexer->position++;
    }

    // If no directive keyword follows, treat this as the stringification operator '#'.
    if (!isalpha((unsigned char)lexer->source[lexer->position]) &&
        lexer->source[lexer->position] != '_') {
        return make_token(lexer, TOKEN_HASH, strndup("#", 1), start);
    }

    // Capture directive keyword (e.g., include, define)
    int directiveStart = lexer->position;
    while (isalnum(lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
        lexer->position++;
    }

    char* directive = strndup(lexer->source + directiveStart, lexer->position - directiveStart);

    // Match known directives
    if (strcmp(directive, "include") == 0) {
        return make_token(lexer, TOKEN_INCLUDE, directive, start);
    } else if (strcmp(directive, "include_next") == 0) {
        return make_token(lexer, TOKEN_INCLUDE_NEXT, directive, start);
    } else if (strcmp(directive, "define") == 0) {
        return make_token(lexer, TOKEN_DEFINE, directive, start);
    } else if (strcmp(directive, "undef") == 0) {
        return make_token(lexer, TOKEN_UNDEF, directive, start);
    } else if (strcmp(directive, "ifdef") == 0) {
        return make_token(lexer, TOKEN_IFDEF, directive, start);
    } else if (strcmp(directive, "ifndef") == 0) {
        return make_token(lexer, TOKEN_IFNDEF, directive, start);
    } else if (strcmp(directive, "if") == 0) {
        return make_token(lexer, TOKEN_PP_IF, directive, start);
    } else if (strcmp(directive, "elif") == 0) {
        return make_token(lexer, TOKEN_PP_ELIF, directive, start);
    } else if (strcmp(directive, "else") == 0) {
        return make_token(lexer, TOKEN_PP_ELSE, directive, start);
    } else if (strcmp(directive, "endif") == 0) {
        return make_token(lexer, TOKEN_ENDIF, directive, start);
    } else if (strcmp(directive, "pragma") == 0) {
        return make_token(lexer, TOKEN_PRAGMA, directive, start);
    }

    // Fallback: unknown preprocessor directive
    return make_token(lexer, TOKEN_PREPROCESSOR_OTHER, directive, start);
}


// Processes comments (single-line `//` and multi-line `/* */`)
Token handleComment(Lexer* lexer) {
    LexerMark slashStart = lexer_mark_previous(lexer);
    if (lexer->source[lexer->position] == '\0' || lexer->source[lexer->position + 1] == '\0') {
        lexer->position++;
        return make_token(lexer, TOKEN_DIVIDE, strndup("/", 1), slashStart);
    }

    char next = lexer->source[lexer->position + 1];

    if (next == '=') { // Handle `/=`
        lexer->position += 2;
        return make_token(lexer, TOKEN_DIV_ASSIGN, (char*)"/=", slashStart);
    }

    //  **Skip Single-line comments (`// ...`)**
    if (next == '/') {
        lexer->position += 2;
        while (lexer->source[lexer->position] != '\n' && lexer->source[lexer->position] != '\0') {
            lexer->position++;
        }
        return getNextToken(lexer);  //  **Skip the comment and return the next real token**
    }

    //  **Skip Multi-line comments (`/* ... */`)**
    if (next == '*') {
        lexer->position += 2;
        while (!(lexer->source[lexer->position] == '*' && lexer->source[lexer->position + 1] == '/')) {
            if (lexer->source[lexer->position] == '\0') {
                LexerMark errorMark = lexer_mark(lexer);
                return make_token(lexer, TOKEN_UNKNOWN, (char*)"Unterminated comment", errorMark);
            }
            if (lexer->source[lexer->position] == '\n') {
                lexer->position++;
                lexer->line++;
                lexer->lineStart = lexer->position;
                continue;
            }
            lexer->position++;
        }
        lexer->position += 2; // Skip `*/`
        return getNextToken(lexer);  //  **Skip the comment and return the next real token**
    }

    lexer->position++; // consume '/'
    return make_token(lexer, TOKEN_DIVIDE, strndup("/", 1), slashStart); // Just a single '/'
}

// Processes all operator tokens (arithmetic, logical, bitwise, assignments)
Token handleOperator(Lexer* lexer) {
    LexerMark start = lexer_mark(lexer);
    char current = lexer->source[lexer->position];
    lexer->position++; // Move past the current character

    switch (current) {
        case '=':
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return make_token(lexer, TOKEN_EQUAL, (char*)"==", start);
            }
            return make_token(lexer, TOKEN_ASSIGN, (char*)"=", start);

        case '+':
            if (lexer->source[lexer->position] == '+') {
                lexer->position++;
                return make_token(lexer, TOKEN_INCREMENT, (char*)"++", start);
            }
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return make_token(lexer, TOKEN_PLUS_ASSIGN, (char*)"+=", start);
            }
            return make_token(lexer, TOKEN_PLUS, (char*)"+", start);

        case '-':
            if (lexer->source[lexer->position] == '-') {
                lexer->position++;
                return make_token(lexer, TOKEN_DECREMENT, (char*)"--", start);
            }
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return make_token(lexer, TOKEN_MINUS_ASSIGN, (char*)"-=", start);
            }
	    if (lexer->source[lexer->position] == '>') {
                lexer->position++;
                return make_token(lexer, TOKEN_ARROW, (char*)"->", start);
            }
            return make_token(lexer, TOKEN_MINUS, (char*)"-", start);
	case '*':
            //  Check if it's an assignment (`*=`)
            if (lexer->source[lexer->position] == '=') {
                lexer->position++; 
                return make_token(lexer, TOKEN_MULT_ASSIGN, (char*)"*=", start);
            }

            //  Otherwise, always return `*` as TOKEN_ASTERISK
            return make_token(lexer, TOKEN_ASTERISK, (char*)"*", start);

        case '/':
            lexer->position--; // Let comment handler inspect from '/'
            return handleComment(lexer); // Redirects to comment handler

        case '%':
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return make_token(lexer, TOKEN_MOD_ASSIGN, (char*)"%=", start);
            }
            return make_token(lexer, TOKEN_MODULO, (char*)"%", start);

	case '<':
	    if (lexer->source[lexer->position] == '<') {                  // << or <<=
	        lexer->position++;
	        if (lexer->source[lexer->position] == '=') {              // NEW: <<=
	            lexer->position++;
	            return make_token(lexer, TOKEN_LEFT_SHIFT_ASSIGN, (char*)"<<=", start);
	        }
	        return make_token(lexer, TOKEN_LEFT_SHIFT, (char*)"<<", start);
	    }
	    if (lexer->source[lexer->position] == '=') {
	        lexer->position++;
	        return make_token(lexer, TOKEN_LESS_EQUAL, (char*)"<=", start);
	    }
	    return make_token(lexer, TOKEN_LESS, (char*)"<", start);
	
	case '>':
	    if (lexer->source[lexer->position] == '>') {                  // >> or >>=
	        lexer->position++;
	        if (lexer->source[lexer->position] == '=') {              // NEW: >>=
	            lexer->position++;
	            return make_token(lexer, TOKEN_RIGHT_SHIFT_ASSIGN, (char*)">>=", start);
	        }
	        return make_token(lexer, TOKEN_RIGHT_SHIFT, (char*)">>", start);
	    }
	    if (lexer->source[lexer->position] == '=') {
	        lexer->position++;
	        return make_token(lexer, TOKEN_GREATER_EQUAL, (char*)">=", start);
	    }
	    return make_token(lexer, TOKEN_GREATER, (char*)">", start);

        case '!':
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return make_token(lexer, TOKEN_NOT_EQUAL, (char*)"!=", start);
            }
            return make_token(lexer, TOKEN_LOGICAL_NOT, (char*)"!", start);

	case '&':
	    if (lexer->source[lexer->position] == '&') {
	        lexer->position++;
	        return make_token(lexer, TOKEN_LOGICAL_AND, (char*)"&&", start);
	    }
	    if (lexer->source[lexer->position] == '=') {                 // NEW: &=
	        lexer->position++;
	        return make_token(lexer, TOKEN_BITWISE_AND_ASSIGN, (char*)"&=", start);
	    }
	    return make_token(lexer, TOKEN_BITWISE_AND, (char*)"&", start);
	
	case '|':
	    if (lexer->source[lexer->position] == '|') {
	        lexer->position++;
	        return make_token(lexer, TOKEN_LOGICAL_OR, (char*)"||", start);
	    }
	    if (lexer->source[lexer->position] == '=') {                  // NEW: |=
	        lexer->position++;
	        return make_token(lexer, TOKEN_BITWISE_OR_ASSIGN, (char*)"|=", start);
	    }
	    return make_token(lexer, TOKEN_BITWISE_OR, (char*)"|", start);

	case '^':
	    if (lexer->source[lexer->position] == '=') {
	        lexer->position++;
	        return make_token(lexer, TOKEN_BITWISE_XOR_ASSIGN, (char*)"^=", start);
	    }
	    return make_token(lexer, TOKEN_BITWISE_XOR, (char*)"^", start);

	// Call helper methods for single-character operators and punctuation
        case '~': return make_token(lexer, TOKEN_BITWISE_NOT, (char*)"~", start);
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
    LexerMark start = lexer_mark(lexer);
    char current = lexer->source[lexer->position];
    lexer->position++; // Move past the punctuation character

    switch (current) {
	case '?': return make_token(lexer, TOKEN_QUESTION, (char*)"?", start);
        case ';': return make_token(lexer, TOKEN_SEMICOLON, (char*)";", start);
        case ',': return make_token(lexer, TOKEN_COMMA, (char*)",", start);
        case '.': {
            // Detect ellipsis "..." used in variadic parameter lists
            if (lexer->source[lexer->position] == '.' &&
                lexer->source[lexer->position + 1] == '.') {
                lexer->position += 2;
                return make_token(lexer, TOKEN_ELLIPSIS, (char*)"...", start);
            }
            return make_token(lexer, TOKEN_DOT, (char*)".", start);
        }
        case '(': return make_token(lexer, TOKEN_LPAREN, (char*)"(", start);
        case ')': return make_token(lexer, TOKEN_RPAREN, (char*)")", start);
        case '{': return make_token(lexer, TOKEN_LBRACE, (char*)"{", start);
        case '}': return make_token(lexer, TOKEN_RBRACE, (char*)"}", start);
        case '[': return make_token(lexer, TOKEN_LBRACKET, (char*)"[", start);
        case ']': return make_token(lexer, TOKEN_RBRACKET, (char*)"]", start);
        case ':': return make_token(lexer, TOKEN_COLON, (char*)":", start); // Added support for switch-case
        default:
            LEXER_DEBUG_PRINTF("Warning: Unknown punctuation '%c' at line %d\n", current, lexer->line);
            return make_token(lexer, TOKEN_UNKNOWN, (char*)"Unknown punctuation", start);
        }
}


// Handles unknown tokens (invalid characters, etc.)
Token handleUnknownToken(Lexer* lexer) {
    LexerMark start = lexer_mark(lexer);
    lexer->position++; // Move past the current character
    return make_token(lexer, TOKEN_UNKNOWN, (char*)"ERROR", start);
}



TokenType keywordToTokenType(const char* word) {
    if (strcmp(word, "int") == 0) return TOKEN_INT;
    if (strcmp(word, "char") == 0) return TOKEN_CHAR;
    if (strcmp(word, "float") == 0) return TOKEN_FLOAT;
    if (strcmp(word, "double") == 0) return TOKEN_DOUBLE;
    if (strcmp(word, "bool") == 0) return TOKEN_BOOL;
    if (strcmp(word, "true") == 0) return TOKEN_TRUE;
    if (strcmp(word, "false") == 0) return TOKEN_FALSE;
    if (strcmp(word, "long") == 0) return TOKEN_LONG;
    if (strcmp(word, "short") == 0) return TOKEN_SHORT;
    if (strcmp(word, "signed") == 0) return TOKEN_SIGNED;
    if (strcmp(word, "__signed") == 0 || strcmp(word, "__signed__") == 0) return TOKEN_SIGNED;
    if (strcmp(word, "unsigned") == 0) return TOKEN_UNSIGNED;
    if (strcmp(word, "enum") == 0) return TOKEN_ENUM;
    if (strcmp(word, "union") == 0) return TOKEN_UNION;
    if (strcmp(word, "struct") == 0) return TOKEN_STRUCT;
    if (strcmp(word, "typedef") == 0) return TOKEN_TYPEDEF;
    if (strcmp(word, "void") == 0) return TOKEN_VOID;
    if (strcmp(word, "__inline") == 0 || strcmp(word, "__inline__") == 0) return TOKEN_INLINE;
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
    if (strcmp(word, "__const") == 0 || strcmp(word, "__const__") == 0) return TOKEN_CONST;
    if (strcmp(word, "volatile") == 0) return TOKEN_VOLATILE;
    if (strcmp(word, "restrict") == 0 || strcmp(word, "__restrict") == 0 || strcmp(word, "__restrict__") == 0) return TOKEN_RESTRICT;
    if (strcmp(word, "inline") == 0) return TOKEN_INLINE;
    if (strcmp(word, "sizeof") == 0) return TOKEN_SIZEOF;
    if (strcmp(word, "asm") == 0) return TOKEN_ASM;
    if (strcmp(word, "pragma") == 0) return TOKEN_PRAGMA;
    if (strcmp(word, "once") == 0) return TOKEN_ONCE;
    return TOKEN_IDENTIFIER;
}
