// SPDX-License-Identifier: Apache-2.0

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer_internal.h"
#include "Lexer/keyword_lookup.h"

static void consume_char_literal_tail_after_error(Lexer* lexer) {
    if (!lexer) return;
    while (!isEOF(lexer)) {
        char c = lexer->source[lexer->position];
        if (c == '\'') {
            lexer->position++;
            return;
        }
        if (c == '\n') {
            return;
        }
        lexer->position++;
    }
}

static void consume_string_literal_tail_after_error(Lexer* lexer) {
    if (!lexer) return;
    while (!isEOF(lexer)) {
        char c = lexer->source[lexer->position];
        if (c == '"') {
            lexer->position++;
            return;
        }
        if (c == '\n') {
            return;
        }
        if (c == '\\') {
            lexer->position++;
            if (isEOF(lexer) || lexer->source[lexer->position] == '\n') {
                return;
            }
            lexer->position++;
            continue;
        }
        lexer->position++;
    }
}

static void format_simple_escape(char out[4], char escapeChar) {
    if (!out) return;
    out[0] = '\\';
    out[1] = escapeChar;
    out[2] = '\0';
    out[3] = '\0';
}

const char *lookupKeyword(const char *str, size_t len) {
    return in_keyword_set(str, len);
}

Token handleIdentifierOrKeyword(Lexer* lexer) {
    LexerMark startMark = lexer_mark(lexer);
    int startPos = lexer->position;

    while (isalnum((unsigned char)lexer->source[lexer->position]) ||
           lexer->source[lexer->position] == '_') {
        lexer->position++;
    }

    char* text = strndup(lexer->source + startPos, lexer->position - startPos);
    if (lexer_debug_enabled()) {
        LEXER_DEBUG_PRINTF("DEBUG: Identified potential identifier or keyword: %s\n", text);
    }

    const char* matchedKeyword = lookupKeyword(text, strlen(text));
    if (matchedKeyword) {
        TokenType tokenType = keywordToTokenType(matchedKeyword);
        if (lexer_debug_enabled()) {
            LEXER_DEBUG_PRINTF("DEBUG: Matched keyword: %s -> TokenType: %d\n", text, tokenType);
        }
        return make_token(lexer, tokenType, text, startMark);
    }

    if (strcmp(text, "__signed") == 0 || strcmp(text, "__signed__") == 0) {
        return make_token(lexer, TOKEN_SIGNED, text, startMark);
    }
    if (strcmp(text, "_Float16") == 0 || strcmp(text, "__fp16") == 0) {
        return make_token(lexer, TOKEN_FLOAT, text, startMark);
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
    if (strcmp(text, "__complex") == 0 || strcmp(text, "__complex__") == 0) {
        return make_token(lexer, TOKEN_COMPLEX, text, startMark);
    }
    if (strcmp(text, "__imaginary") == 0 || strcmp(text, "__imaginary__") == 0) {
        return make_token(lexer, TOKEN_IMAGINARY, text, startMark);
    }

    if (lexer_debug_enabled()) {
        LEXER_DEBUG_PRINTF("DEBUG: Classified as identifier: %s\n", text);
    }

    return make_token(lexer, TOKEN_IDENTIFIER, text, startMark);
}

Token handleNumber(Lexer* lexer) {
    LexerMark startMark = lexer_mark(lexer);
    int startPos = lexer->position;
    TokenType type = TOKEN_NUMBER;

    bool isHex = false;
    bool isBinary = false;
    bool sawFraction = false;
    bool invalidOctalDigit = false;
    bool invalidBinaryDigit = false;
    int radixDigits = 0;
    int hexFractionDigits = 0;

    if (lexer->source[startPos] == '0') {
        if (lexer->source[startPos + 1] == 'x' || lexer->source[startPos + 1] == 'X') {
            lexer->position += 2;
            isHex = true;
            while (isxdigit((unsigned char)lexer->source[lexer->position])) {
                radixDigits++;
                lexer->position++;
            }
            type = TOKEN_NUMBER;
        } else if (lexer->source[startPos + 1] == 'b' || lexer->source[startPos + 1] == 'B') {
            lexer->position += 2;
            isBinary = true;
            while (lexer->source[lexer->position] == '0' || lexer->source[lexer->position] == '1') {
                radixDigits++;
                lexer->position++;
            }
            while (isdigit((unsigned char)lexer->source[lexer->position])) {
                invalidBinaryDigit = true;
                lexer->position++;
            }
            type = TOKEN_NUMBER;
        } else {
            while (isdigit((unsigned char)lexer->source[lexer->position])) {
                if (lexer->source[lexer->position] == '8' || lexer->source[lexer->position] == '9') {
                    invalidOctalDigit = true;
                }
                lexer->position++;
            }
            type = TOKEN_NUMBER;
        }
    } else {
        while (isdigit((unsigned char)lexer->source[lexer->position])) lexer->position++;
    }

    bool sawExp = false;
    if (lexer->source[lexer->position] == '.') {
        lexer->position++;
        sawFraction = true;
        if (isHex) {
            while (isxdigit((unsigned char)lexer->source[lexer->position])) {
                hexFractionDigits++;
                lexer->position++;
            }
        } else {
            while (isdigit((unsigned char)lexer->source[lexer->position])) lexer->position++;
        }
        type = TOKEN_FLOAT_LITERAL;
    }
    if (!sawExp && ((!isHex && (lexer->source[lexer->position] == 'e' || lexer->source[lexer->position] == 'E')) ||
                    (isHex && (lexer->source[lexer->position] == 'p' || lexer->source[lexer->position] == 'P')))) {
        sawExp = true;
        lexer->position++;
        if (lexer->source[lexer->position] == '+' || lexer->source[lexer->position] == '-') {
            lexer->position++;
        }
        bool hadDigit = false;
        while (isdigit((unsigned char)lexer->source[lexer->position])) {
            hadDigit = true;
            lexer->position++;
        }
        if (!hadDigit) {
            while (isalnum((unsigned char)lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
                lexer->position++;
            }
            char* text = strndup(lexer->source + startPos, lexer->position - startPos);
            report_lexer_error(lexer, startMark, "Malformed floating literal exponent", text);
            return make_token(lexer, TOKEN_UNKNOWN, text, startMark);
        }
        type = TOKEN_FLOAT_LITERAL;
    }

    bool seenU = false;
    int lCount = 0;
    while (1) {
        char c = lexer->source[lexer->position];
        if (c == 'u' || c == 'U') {
            if (seenU) break;
            seenU = true;
            lexer->position++;
            continue;
        }
        if (c == 'l' || c == 'L') {
            if (lCount >= 2) break;
            lCount++;
            lexer->position++;
            continue;
        }
        if (c == 'f' || c == 'F') {
            type = TOKEN_FLOAT_LITERAL;
            lexer->position++;
            continue;
        }
        if (c == 'i' || c == 'I' || c == 'j' || c == 'J') {
            type = TOKEN_FLOAT_LITERAL;
            lexer->position++;
            continue;
        }
        break;
    }

    if (isBinary && (sawFraction || sawExp || type == TOKEN_FLOAT_LITERAL ||
                     lexer->source[lexer->position] == 'p' || lexer->source[lexer->position] == 'P')) {
        char* text = strndup(lexer->source + startPos, lexer->position - startPos);
        if (lexer->source[lexer->position] == 'p' || lexer->source[lexer->position] == 'P') {
            while (isalnum((unsigned char)lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
                lexer->position++;
            }
            free(text);
            text = strndup(lexer->source + startPos, lexer->position - startPos);
        }
        report_lexer_error(lexer, startMark, "Binary literals do not support fractional or exponent forms", text);
        return make_token(lexer, TOKEN_UNKNOWN, text, startMark);
    }

    if (isBinary && invalidBinaryDigit) {
        char* text = strndup(lexer->source + startPos, lexer->position - startPos);
        report_lexer_error(lexer, startMark, "Invalid digit in binary literal", text);
        return make_token(lexer, TOKEN_UNKNOWN, text, startMark);
    }

    if (isBinary && radixDigits == 0) {
        char* text = strndup(lexer->source + startPos, lexer->position - startPos);
        report_lexer_error(lexer, startMark, "Binary literal requires at least one binary digit", text);
        return make_token(lexer, TOKEN_UNKNOWN, text, startMark);
    }

    if (!isHex && !isBinary && invalidOctalDigit && !sawFraction && !sawExp && type == TOKEN_NUMBER) {
        char* text = strndup(lexer->source + startPos, lexer->position - startPos);
        report_lexer_error(lexer, startMark, "Invalid digit in octal literal", text);
        return make_token(lexer, TOKEN_UNKNOWN, text, startMark);
    }

    if (isHex && !sawFraction && radixDigits == 0) {
        char* text = strndup(lexer->source + startPos, lexer->position - startPos);
        report_lexer_error(lexer, startMark, "Hex literal requires at least one hex digit", text);
        return make_token(lexer, TOKEN_UNKNOWN, text, startMark);
    }

    if (isHex && sawFraction && radixDigits == 0 && hexFractionDigits == 0) {
        char* text = strndup(lexer->source + startPos, lexer->position - startPos);
        report_lexer_error(lexer, startMark, "Hex floating literal requires at least one mantissa digit", text);
        return make_token(lexer, TOKEN_UNKNOWN, text, startMark);
    }

    if (isHex && sawFraction && !sawExp) {
        char* text = strndup(lexer->source + startPos, lexer->position - startPos);
        report_lexer_error(lexer, startMark, "Hex floating literal requires a 'p' exponent", text);
        return make_token(lexer, TOKEN_UNKNOWN, text, startMark);
    }

    if (isalpha((unsigned char)lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
        while (isalnum((unsigned char)lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
            lexer->position++;
        }
        char* text = strndup(lexer->source + startPos, lexer->position - startPos);
        report_lexer_error(lexer, startMark, "Invalid numeric literal suffix", text);
        return make_token(lexer, TOKEN_UNKNOWN, text, startMark);
    }

    return make_token(lexer, type, strndup(lexer->source + startPos, lexer->position - startPos), startMark);
}

Token handleStringLiteral(Lexer* lexer) {
    LexerMark startMark = lexer_mark(lexer);
    lexer->position++;
    int start = lexer->position;

    while (1) {
        char c = lexer->source[lexer->position];
        if (c == '\0' || c == '\n') {
            report_lexer_error(lexer, startMark, "Unterminated string literal", "\"");
            return make_token(lexer, TOKEN_UNKNOWN, (char*)"Unterminated string", startMark);
        }
        if (c == '"') {
            break;
        }
        if (c == '\\') {
            lexer->position++;
            char e = lexer->source[lexer->position];
            if (e == '\0' || e == '\n') {
                report_lexer_error(lexer, startMark, "Unterminated string literal", "\"");
                return make_token(lexer, TOKEN_UNKNOWN, (char*)"Unterminated string", startMark);
            }
            switch (e) {
                case 'n':
                case 't':
                case 'r':
                case 'b':
                case 'f':
                case 'a':
                case 'v':
                case '\\':
                case '\'':
                case '\"':
                case '?':
                    lexer->position++;
                    break;
                case 'x': {
                    lexer->position++;
                    if (!isxdigit((unsigned char)lexer->source[lexer->position])) {
                        report_lexer_error(lexer, startMark, "Invalid \\x escape in string literal", "\\x");
                        consume_string_literal_tail_after_error(lexer);
                        return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid \\x escape", startMark);
                    }
                    while (isxdigit((unsigned char)lexer->source[lexer->position])) {
                        lexer->position++;
                    }
                    break;
                }
                case 'u': {
                    lexer->position++;
                    int countHex = 0;
                    while (countHex < 4 && isxdigit((unsigned char)lexer->source[lexer->position])) {
                        lexer->position++;
                        countHex++;
                    }
                    if (countHex != 4) {
                        report_lexer_error(lexer, startMark, "Invalid \\u escape in string literal", "\\u");
                        consume_string_literal_tail_after_error(lexer);
                        return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid \\u escape", startMark);
                    }
                    break;
                }
                case 'U': {
                    lexer->position++;
                    int countHex = 0;
                    while (countHex < 8 && isxdigit((unsigned char)lexer->source[lexer->position])) {
                        lexer->position++;
                        countHex++;
                    }
                    if (countHex != 8) {
                        report_lexer_error(lexer, startMark, "Invalid \\U escape in string literal", "\\U");
                        consume_string_literal_tail_after_error(lexer);
                        return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid \\U escape", startMark);
                    }
                    break;
                }
                default:
                    if (e >= '0' && e <= '7') {
                        lexer->position++;
                        for (int k = 0; k < 2; ++k) {
                            char d = lexer->source[lexer->position];
                            if (d < '0' || d > '7') break;
                            lexer->position++;
                        }
                    } else {
                        char got[4];
                        format_simple_escape(got, e);
                        report_lexer_error(lexer, startMark, "Invalid escape in string literal", got);
                        consume_string_literal_tail_after_error(lexer);
                        return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid escape", startMark);
                    }
                    break;
            }
            continue;
        }
        lexer->position++;
    }

    char* text = strndup(lexer->source + start, lexer->position - start);
    lexer->position++;
    return make_token(lexer, TOKEN_STRING, text, startMark);
}

Token handleCharLiteral(Lexer* lexer) {
    LexerMark startMark = lexer_mark(lexer);
    LEXER_DEBUG_PRINTF("DEBUG: Entering handleCharLiteral() at line %d\n", lexer->line);
    lexer->position++;
    uint64_t accum = 0;
    int count = 0;
    int lastVal = 0;

    while (1) {
        char ch = lexer->source[lexer->position];
        if (ch == '\0' || ch == '\n') {
            report_lexer_error(lexer, startMark, "Unterminated character literal", "'");
            return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid character literal", startMark);
        }
        if (ch == '\'') {
            lexer->position++;
            break;
        }

        lexer->position++;
        int val = 0;
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
                case '?':  val = '?'; break;
                case 'u': {
                    int hex = 0;
                    int countHex = 0;
                    for (int k = 0; k < 4; ++k) {
                        char h = lexer->source[lexer->position++];
                        int v = 0;
                        if (h >= '0' && h <= '9') v = h - '0';
                        else if (h >= 'a' && h <= 'f') v = h - 'a' + 10;
                        else if (h >= 'A' && h <= 'F') v = h - 'A' + 10;
                        else { lexer->position--; break; }
                        hex = (hex << 4) | v;
                        ++countHex;
                    }
                    if (countHex != 4) {
                        report_lexer_error(lexer, startMark, "Invalid \\u escape in character literal", "\\u");
                        consume_char_literal_tail_after_error(lexer);
                        return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid \\u escape", startMark);
                    }
                    val = hex;
                    break;
                }
                case 'U': {
                    int hex = 0;
                    int countHex = 0;
                    for (int k = 0; k < 8; ++k) {
                        char h = lexer->source[lexer->position++];
                        int v = 0;
                        if (h >= '0' && h <= '9') v = h - '0';
                        else if (h >= 'a' && h <= 'f') v = h - 'a' + 10;
                        else if (h >= 'A' && h <= 'F') v = h - 'A' + 10;
                        else { lexer->position--; break; }
                        hex = (hex << 4) | v;
                        ++countHex;
                    }
                    if (countHex != 8) {
                        report_lexer_error(lexer, startMark, "Invalid \\U escape in character literal", "\\U");
                        consume_char_literal_tail_after_error(lexer);
                        return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid \\U escape", startMark);
                    }
                    val = hex;
                    break;
                }
                case 'x': {
                    int hex = 0;
                    int any = 0;
                    while (isxdigit((unsigned char)lexer->source[lexer->position])) {
                        char h = lexer->source[lexer->position++];
                        hex *= 16;
                        if (h >= '0' && h <= '9') hex += (h - '0');
                        else if (h >= 'a' && h <= 'f') hex += (h - 'a' + 10);
                        else hex += (h - 'A' + 10);
                        any = 1;
                    }
                    if (!any) {
                        report_lexer_error(lexer, startMark, "Invalid \\x escape in character literal", "\\x");
                        consume_char_literal_tail_after_error(lexer);
                        return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid \\x escape", startMark);
                    }
                    val = hex;
                    break;
                }
                default:
                    if (e >= '0' && e <= '7') {
                        int oct = (e - '0');
                        for (int k = 0; k < 2; ++k) {
                            char d = lexer->source[lexer->position];
                            if (d < '0' || d > '7') break;
                            lexer->position++;
                            oct = (oct << 3) + (d - '0');
                        }
                        val = oct;
                    } else {
                        char got[4];
                        format_simple_escape(got, e);
                        report_lexer_error(lexer, startMark, "Invalid escape in character literal", got);
                        consume_char_literal_tail_after_error(lexer);
                        return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid escape", startMark);
                    }
                    break;
            }
        } else {
            val = (unsigned char)ch;
        }

        lastVal = val;
        if (count == 0) {
            accum = (uint64_t)(val & 0xFF);
        } else {
            accum = (accum << 8) | (uint64_t)(val & 0xFF);
        }
        count++;
    }

    if (count == 0) {
        report_lexer_error(lexer, startMark, "Empty character literal", "''");
        return make_token(lexer, TOKEN_UNKNOWN, (char*)"Invalid character literal", startMark);
    }

    uint64_t finalVal = (count == 1) ? (uint64_t)lastVal : accum;
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)finalVal);
    LEXER_DEBUG_PRINTF("DEBUG: Created TOKEN_CHAR_LITERAL with value %s at line %d\n", buf, lexer->line);
    return make_token(lexer, TOKEN_CHAR_LITERAL, strdup(buf), startMark);
}

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
    lexer->position++;

    if (!atLineStart) {
        if (lexer->source[lexer->position] == '#') {
            lexer->position++;
            return make_token(lexer, TOKEN_DOUBLE_HASH, strndup("##", 2), start);
        }
        return make_token(lexer, TOKEN_HASH, strndup("#", 1), start);
    }

    while (isspace((unsigned char)lexer->source[lexer->position])) {
        if (lexer->source[lexer->position] == '\n') {
            lexer->position++;
            lexer->line++;
            lexer->lineStart = lexer->position;
            continue;
        }
        lexer->position++;
    }

    if (!isalpha((unsigned char)lexer->source[lexer->position]) &&
        lexer->source[lexer->position] != '_') {
        return make_token(lexer, TOKEN_HASH, strndup("#", 1), start);
    }

    int directiveStart = lexer->position;
    while (isalnum((unsigned char)lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
        lexer->position++;
    }

    char* directive = strndup(lexer->source + directiveStart, lexer->position - directiveStart);

    if (strcmp(directive, "include") == 0) return make_token(lexer, TOKEN_INCLUDE, directive, start);
    if (strcmp(directive, "include_next") == 0) return make_token(lexer, TOKEN_INCLUDE_NEXT, directive, start);
    if (strcmp(directive, "define") == 0) return make_token(lexer, TOKEN_DEFINE, directive, start);
    if (strcmp(directive, "undef") == 0) return make_token(lexer, TOKEN_UNDEF, directive, start);
    if (strcmp(directive, "ifdef") == 0) return make_token(lexer, TOKEN_IFDEF, directive, start);
    if (strcmp(directive, "ifndef") == 0) return make_token(lexer, TOKEN_IFNDEF, directive, start);
    if (strcmp(directive, "if") == 0) return make_token(lexer, TOKEN_PP_IF, directive, start);
    if (strcmp(directive, "elif") == 0) return make_token(lexer, TOKEN_PP_ELIF, directive, start);
    if (strcmp(directive, "else") == 0) return make_token(lexer, TOKEN_PP_ELSE, directive, start);
    if (strcmp(directive, "endif") == 0) return make_token(lexer, TOKEN_ENDIF, directive, start);
    if (strcmp(directive, "pragma") == 0) return make_token(lexer, TOKEN_PRAGMA, directive, start);

    return make_token(lexer, TOKEN_PREPROCESSOR_OTHER, directive, start);
}

Token handleComment(Lexer* lexer) {
    LexerMark slashStart = lexer_mark(lexer);
    if (lexer->source[lexer->position] == '\0' || lexer->source[lexer->position + 1] == '\0') {
        lexer->position++;
        return make_token(lexer, TOKEN_DIVIDE, strndup("/", 1), slashStart);
    }

    char next = lexer->source[lexer->position + 1];

    if (next == '=') {
        lexer->position += 2;
        return make_token(lexer, TOKEN_DIV_ASSIGN, (char*)"/=", slashStart);
    }

    if (next == '/') {
        lexer->position += 2;
        while (lexer->source[lexer->position] != '\n' && lexer->source[lexer->position] != '\0') {
            lexer->position++;
        }
        return getNextToken(lexer);
    }

    if (next == '*') {
        lexer->position += 2;
        while (!(lexer->source[lexer->position] == '*' && lexer->source[lexer->position + 1] == '/')) {
            if (lexer->source[lexer->position] == '\0') {
                report_lexer_error(lexer, slashStart, "Unterminated block comment", "/*");
                return make_token(lexer, TOKEN_UNKNOWN, (char*)"Unterminated comment", slashStart);
            }
            if (lexer->source[lexer->position] == '\n') {
                lexer->position++;
                lexer->line++;
                lexer->lineStart = lexer->position;
                continue;
            }
            lexer->position++;
        }
        lexer->position += 2;
        return getNextToken(lexer);
    }

    lexer->position++;
    return make_token(lexer, TOKEN_DIVIDE, strndup("/", 1), slashStart);
}

Token handleOperator(Lexer* lexer) {
    LexerMark start = lexer_mark(lexer);
    char current = lexer->source[lexer->position];
    lexer->position++;

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
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return make_token(lexer, TOKEN_MULT_ASSIGN, (char*)"*=", start);
            }
            return make_token(lexer, TOKEN_ASTERISK, (char*)"*", start);
        case '/':
            lexer->position--;
            return handleComment(lexer);
        case '%':
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return make_token(lexer, TOKEN_MOD_ASSIGN, (char*)"%=", start);
            }
            return make_token(lexer, TOKEN_MODULO, (char*)"%", start);
        case '<':
            if (lexer->source[lexer->position] == '<') {
                lexer->position++;
                if (lexer->source[lexer->position] == '=') {
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
            if (lexer->source[lexer->position] == '>') {
                lexer->position++;
                if (lexer->source[lexer->position] == '=') {
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
            if (lexer->source[lexer->position] == '=') {
                lexer->position++;
                return make_token(lexer, TOKEN_BITWISE_AND_ASSIGN, (char*)"&=", start);
            }
            return make_token(lexer, TOKEN_BITWISE_AND, (char*)"&", start);
        case '|':
            if (lexer->source[lexer->position] == '|') {
                lexer->position++;
                return make_token(lexer, TOKEN_LOGICAL_OR, (char*)"||", start);
            }
            if (lexer->source[lexer->position] == '=') {
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
        case '~': return make_token(lexer, TOKEN_BITWISE_NOT, (char*)"~", start);
        case ';':
        case ',':
        case '.':
        case '(':
        case ')':
        case '{':
        case '}':
        case '[':
        case ']':
            lexer->position--;
            return handlePunctuation(lexer);
        default:
            return handleUnknownToken(lexer);
    }
}

Token handlePunctuation(Lexer* lexer) {
    LexerMark start = lexer_mark(lexer);
    char current = lexer->source[lexer->position];
    lexer->position++;

    switch (current) {
        case '?': return make_token(lexer, TOKEN_QUESTION, (char*)"?", start);
        case ';': return make_token(lexer, TOKEN_SEMICOLON, (char*)";", start);
        case ',': return make_token(lexer, TOKEN_COMMA, (char*)",", start);
        case '.':
            if (lexer->source[lexer->position] == '.' &&
                lexer->source[lexer->position + 1] == '.') {
                lexer->position += 2;
                return make_token(lexer, TOKEN_ELLIPSIS, (char*)"...", start);
            }
            return make_token(lexer, TOKEN_DOT, (char*)".", start);
        case '(': return make_token(lexer, TOKEN_LPAREN, (char*)"(", start);
        case ')': return make_token(lexer, TOKEN_RPAREN, (char*)")", start);
        case '{': return make_token(lexer, TOKEN_LBRACE, (char*)"{", start);
        case '}': return make_token(lexer, TOKEN_RBRACE, (char*)"}", start);
        case '[': return make_token(lexer, TOKEN_LBRACKET, (char*)"[", start);
        case ']': return make_token(lexer, TOKEN_RBRACKET, (char*)"]", start);
        case ':': return make_token(lexer, TOKEN_COLON, (char*)":", start);
        default:
            LEXER_DEBUG_PRINTF("Warning: Unknown punctuation '%c' at line %d\n", current, lexer->line);
            return make_token(lexer, TOKEN_UNKNOWN, (char*)"Unknown punctuation", start);
    }
}

Token handleUnknownToken(Lexer* lexer) {
    LexerMark start = lexer_mark(lexer);
    unsigned char bad = (unsigned char)lexer->source[lexer->position];
    if (bad == '@') {
        lexer->position++;
        if (lexer_is_system_header_path(lexer_file_path(lexer))) {
            return make_token(lexer, TOKEN_UNKNOWN, (char*)"@", start);
        }
        report_lexer_error(lexer, start, "Invalid character in source", "@");
        return make_token(lexer, TOKEN_UNKNOWN, (char*)"ERROR", start);
    }
    char got[8];
    if (isprint(bad)) {
        got[0] = (char)bad;
        got[1] = '\0';
    } else {
        snprintf(got, sizeof(got), "\\x%02X", bad);
    }
    report_lexer_error(lexer, start, "Invalid character in source", got);
    lexer->position++;
    return make_token(lexer, TOKEN_UNKNOWN, (char*)"ERROR", start);
}

TokenType keywordToTokenType(const char* word) {
    if (strcmp(word, "int") == 0) return TOKEN_INT;
    if (strcmp(word, "char") == 0) return TOKEN_CHAR;
    if (strcmp(word, "float") == 0) return TOKEN_FLOAT;
    if (strcmp(word, "double") == 0) return TOKEN_DOUBLE;
    if (strcmp(word, "_Bool") == 0) return TOKEN_BOOL;
    if (strcmp(word, "_Complex") == 0 || strcmp(word, "__complex") == 0 || strcmp(word, "__complex__") == 0) return TOKEN_COMPLEX;
    if (strcmp(word, "_Imaginary") == 0 || strcmp(word, "__imaginary") == 0 || strcmp(word, "__imaginary__") == 0) return TOKEN_IMAGINARY;
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
    if (strcmp(word, "_Thread_local") == 0) return TOKEN_THREAD_LOCAL;
    if (strcmp(word, "const") == 0) return TOKEN_CONST;
    if (strcmp(word, "__const") == 0 || strcmp(word, "__const__") == 0) return TOKEN_CONST;
    if (strcmp(word, "volatile") == 0) return TOKEN_VOLATILE;
    if (strcmp(word, "restrict") == 0 || strcmp(word, "__restrict") == 0 || strcmp(word, "__restrict__") == 0) return TOKEN_RESTRICT;
    if (strcmp(word, "inline") == 0) return TOKEN_INLINE;
    if (strcmp(word, "_Atomic") == 0) return TOKEN_ATOMIC;
    if (strcmp(word, "sizeof") == 0) return TOKEN_SIZEOF;
    if (strcmp(word, "_Alignof") == 0 || strcmp(word, "alignof") == 0) return TOKEN_ALIGNOF;
    if (strcmp(word, "_Static_assert") == 0) return TOKEN_STATIC_ASSERT;
    if (strcmp(word, "asm") == 0) return TOKEN_ASM;
    if (strcmp(word, "pragma") == 0) return TOKEN_PRAGMA;
    if (strcmp(word, "once") == 0) return TOKEN_ONCE;
    return TOKEN_IDENTIFIER;
}
