// SPDX-License-Identifier: Apache-2.0

#include "Compiler/semantic_project_json.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FISICS_JSON_MAX_DEPTH 64u

typedef struct JsonParser {
    const char* text;
    size_t length;
    size_t position;
    unsigned depth;
    FisicsJsonDiagnostic* diagnostic;
} JsonParser;

static void json_error(JsonParser* parser, const char* format, ...) {
    if (!parser || !parser->diagnostic || parser->diagnostic->message[0]) return;
    va_list args;
    va_start(args, format);
    char detail[192];
    vsnprintf(detail, sizeof(detail), format, args);
    va_end(args);
    snprintf(parser->diagnostic->message,
             sizeof(parser->diagnostic->message),
             "JSON byte %zu: %s",
             parser->position,
             detail);
}

static void skip_whitespace(JsonParser* parser) {
    while (parser->position < parser->length) {
        char c = parser->text[parser->position];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
        parser->position++;
    }
}

static bool append_byte(char** buffer, size_t* length, size_t* capacity, unsigned char byte) {
    if (*length + 1u >= *capacity) {
        size_t next_capacity = *capacity ? *capacity * 2u : 32u;
        if (next_capacity <= *length || next_capacity > SIZE_MAX / 2u) return false;
        char* next = (char*)realloc(*buffer, next_capacity);
        if (!next) return false;
        *buffer = next;
        *capacity = next_capacity;
    }
    (*buffer)[(*length)++] = (char)byte;
    return true;
}

static bool append_utf8(char** buffer,
                        size_t* length,
                        size_t* capacity,
                        uint32_t codepoint) {
    if (codepoint == 0u) return false;
    if (codepoint <= 0x7fu) {
        return append_byte(buffer, length, capacity, (unsigned char)codepoint);
    }
    if (codepoint <= 0x7ffu) {
        return append_byte(buffer, length, capacity,
                           (unsigned char)(0xc0u | (codepoint >> 6u))) &&
               append_byte(buffer, length, capacity,
                           (unsigned char)(0x80u | (codepoint & 0x3fu)));
    }
    if (codepoint <= 0xffffu) {
        if (codepoint >= 0xd800u && codepoint <= 0xdfffu) return false;
        return append_byte(buffer, length, capacity,
                           (unsigned char)(0xe0u | (codepoint >> 12u))) &&
               append_byte(buffer, length, capacity,
                           (unsigned char)(0x80u | ((codepoint >> 6u) & 0x3fu))) &&
               append_byte(buffer, length, capacity,
                           (unsigned char)(0x80u | (codepoint & 0x3fu)));
    }
    if (codepoint <= 0x10ffffu) {
        return append_byte(buffer, length, capacity,
                           (unsigned char)(0xf0u | (codepoint >> 18u))) &&
               append_byte(buffer, length, capacity,
                           (unsigned char)(0x80u | ((codepoint >> 12u) & 0x3fu))) &&
               append_byte(buffer, length, capacity,
                           (unsigned char)(0x80u | ((codepoint >> 6u) & 0x3fu))) &&
               append_byte(buffer, length, capacity,
                           (unsigned char)(0x80u | (codepoint & 0x3fu)));
    }
    return false;
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool parse_hex4(JsonParser* parser, uint32_t* out) {
    if (parser->length - parser->position < 4u) return false;
    uint32_t value = 0;
    for (unsigned i = 0; i < 4u; ++i) {
        int digit = hex_digit(parser->text[parser->position++]);
        if (digit < 0) return false;
        value = (value << 4u) | (uint32_t)digit;
    }
    *out = value;
    return true;
}

static bool valid_utf8_sequence(const unsigned char* bytes,
                                size_t available,
                                size_t* out_count) {
    if (!available) return false;
    unsigned char c = bytes[0];
    if (c < 0x80u) {
        *out_count = 1u;
        return c >= 0x20u;
    }
    if (c >= 0xc2u && c <= 0xdfu && available >= 2u &&
        (bytes[1] & 0xc0u) == 0x80u) {
        *out_count = 2u;
        return true;
    }
    if (c >= 0xe0u && c <= 0xefu && available >= 3u &&
        (bytes[1] & 0xc0u) == 0x80u && (bytes[2] & 0xc0u) == 0x80u) {
        if (c == 0xe0u && bytes[1] < 0xa0u) return false;
        if (c == 0xedu && bytes[1] >= 0xa0u) return false;
        *out_count = 3u;
        return true;
    }
    if (c >= 0xf0u && c <= 0xf4u && available >= 4u &&
        (bytes[1] & 0xc0u) == 0x80u && (bytes[2] & 0xc0u) == 0x80u &&
        (bytes[3] & 0xc0u) == 0x80u) {
        if (c == 0xf0u && bytes[1] < 0x90u) return false;
        if (c == 0xf4u && bytes[1] >= 0x90u) return false;
        *out_count = 4u;
        return true;
    }
    return false;
}

static char* parse_string(JsonParser* parser) {
    if (parser->position >= parser->length || parser->text[parser->position] != '"') {
        json_error(parser, "expected string");
        return NULL;
    }
    parser->position++;
    char* buffer = NULL;
    size_t length = 0;
    size_t capacity = 0;
    while (parser->position < parser->length) {
        unsigned char c = (unsigned char)parser->text[parser->position++];
        if (c == '"') {
            if (!append_byte(&buffer, &length, &capacity, 0u)) {
                free(buffer);
                json_error(parser, "out of memory while parsing string");
                return NULL;
            }
            return buffer;
        }
        if (c == '\\') {
            if (parser->position >= parser->length) break;
            char escape = parser->text[parser->position++];
            switch (escape) {
                case '"': c = '"'; break;
                case '\\': c = '\\'; break;
                case '/': c = '/'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'u': {
                    uint32_t codepoint = 0;
                    if (!parse_hex4(parser, &codepoint)) {
                        free(buffer);
                        json_error(parser, "invalid Unicode escape");
                        return NULL;
                    }
                    if (codepoint >= 0xd800u && codepoint <= 0xdbffu) {
                        if (parser->length - parser->position < 6u ||
                            parser->text[parser->position] != '\\' ||
                            parser->text[parser->position + 1u] != 'u') {
                            free(buffer);
                            json_error(parser, "unpaired high surrogate");
                            return NULL;
                        }
                        parser->position += 2u;
                        uint32_t low = 0;
                        if (!parse_hex4(parser, &low) || low < 0xdc00u || low > 0xdfffu) {
                            free(buffer);
                            json_error(parser, "invalid low surrogate");
                            return NULL;
                        }
                        codepoint = 0x10000u + ((codepoint - 0xd800u) << 10u) +
                                    (low - 0xdc00u);
                    } else if (codepoint >= 0xdc00u && codepoint <= 0xdfffu) {
                        free(buffer);
                        json_error(parser, "unpaired low surrogate");
                        return NULL;
                    }
                    if (!append_utf8(&buffer, &length, &capacity, codepoint)) {
                        free(buffer);
                        json_error(parser, "out of memory while parsing Unicode escape");
                        return NULL;
                    }
                    continue;
                }
                default:
                    free(buffer);
                    json_error(parser, "unsupported string escape");
                    return NULL;
            }
            if (!append_byte(&buffer, &length, &capacity, c)) {
                free(buffer);
                json_error(parser, "out of memory while parsing string");
                return NULL;
            }
            continue;
        }
        size_t count = 0;
        if (!valid_utf8_sequence((const unsigned char*)parser->text + parser->position - 1u,
                                 parser->length - parser->position + 1u,
                                 &count)) {
            free(buffer);
            json_error(parser, "invalid UTF-8 or control byte in string");
            return NULL;
        }
        for (size_t i = 0; i < count; ++i) {
            if (!append_byte(&buffer, &length, &capacity,
                             (unsigned char)parser->text[parser->position - 1u + i])) {
                free(buffer);
                json_error(parser, "out of memory while parsing string");
                return NULL;
            }
        }
        parser->position += count - 1u;
    }
    free(buffer);
    json_error(parser, "unterminated string");
    return NULL;
}

static FisicsJsonValue* parse_value(JsonParser* parser);

static FisicsJsonValue* new_value(JsonParser* parser, FisicsJsonType type) {
    FisicsJsonValue* value = (FisicsJsonValue*)calloc(1u, sizeof(*value));
    if (!value) json_error(parser, "out of memory");
    if (value) value->type = type;
    return value;
}

void fisics_json_free(FisicsJsonValue* value) {
    if (!value) return;
    if (value->type == FISICS_JSON_STRING || value->type == FISICS_JSON_NUMBER) {
        free(value->as.text);
    } else if (value->type == FISICS_JSON_ARRAY) {
        for (size_t i = 0; i < value->as.array.count; ++i) {
            fisics_json_free(value->as.array.items[i]);
        }
        free(value->as.array.items);
    } else if (value->type == FISICS_JSON_OBJECT) {
        for (size_t i = 0; i < value->as.object.count; ++i) {
            free(value->as.object.members[i].key);
            fisics_json_free(value->as.object.members[i].value);
        }
        free(value->as.object.members);
    }
    free(value);
}

static bool array_append(JsonParser* parser,
                         FisicsJsonValue* array,
                         FisicsJsonValue* item) {
    if (array->as.array.count == SIZE_MAX / sizeof(*array->as.array.items)) return false;
    FisicsJsonValue** next = (FisicsJsonValue**)realloc(
        array->as.array.items,
        sizeof(*next) * (array->as.array.count + 1u));
    if (!next) {
        json_error(parser, "out of memory while growing array");
        return false;
    }
    array->as.array.items = next;
    array->as.array.items[array->as.array.count++] = item;
    return true;
}

static FisicsJsonValue* parse_array(JsonParser* parser) {
    parser->position++;
    FisicsJsonValue* array = new_value(parser, FISICS_JSON_ARRAY);
    if (!array) return NULL;
    skip_whitespace(parser);
    if (parser->position < parser->length && parser->text[parser->position] == ']') {
        parser->position++;
        return array;
    }
    while (parser->position < parser->length) {
        FisicsJsonValue* item = parse_value(parser);
        if (!item || !array_append(parser, array, item)) {
            fisics_json_free(item);
            fisics_json_free(array);
            return NULL;
        }
        skip_whitespace(parser);
        if (parser->position < parser->length && parser->text[parser->position] == ']') {
            parser->position++;
            return array;
        }
        if (parser->position >= parser->length || parser->text[parser->position++] != ',') {
            json_error(parser, "expected ',' or ']' in array");
            fisics_json_free(array);
            return NULL;
        }
        skip_whitespace(parser);
    }
    json_error(parser, "unterminated array");
    fisics_json_free(array);
    return NULL;
}

static bool object_has_key(const FisicsJsonValue* object, const char* key) {
    for (size_t i = 0; i < object->as.object.count; ++i) {
        if (strcmp(object->as.object.members[i].key, key) == 0) return true;
    }
    return false;
}

static bool object_append(JsonParser* parser,
                          FisicsJsonValue* object,
                          char* key,
                          FisicsJsonValue* value) {
    if (object_has_key(object, key)) {
        json_error(parser, "duplicate object key '%s'", key);
        return false;
    }
    if (object->as.object.count == SIZE_MAX / sizeof(*object->as.object.members)) return false;
    FisicsJsonMember* next = (FisicsJsonMember*)realloc(
        object->as.object.members,
        sizeof(*next) * (object->as.object.count + 1u));
    if (!next) {
        json_error(parser, "out of memory while growing object");
        return false;
    }
    object->as.object.members = next;
    next[object->as.object.count].key = key;
    next[object->as.object.count].value = value;
    object->as.object.count++;
    return true;
}

static FisicsJsonValue* parse_object(JsonParser* parser) {
    parser->position++;
    FisicsJsonValue* object = new_value(parser, FISICS_JSON_OBJECT);
    if (!object) return NULL;
    skip_whitespace(parser);
    if (parser->position < parser->length && parser->text[parser->position] == '}') {
        parser->position++;
        return object;
    }
    while (parser->position < parser->length) {
        char* key = parse_string(parser);
        if (!key) {
            fisics_json_free(object);
            return NULL;
        }
        skip_whitespace(parser);
        if (parser->position >= parser->length || parser->text[parser->position++] != ':') {
            json_error(parser, "expected ':' after object key");
            free(key);
            fisics_json_free(object);
            return NULL;
        }
        FisicsJsonValue* value = parse_value(parser);
        if (!value || !object_append(parser, object, key, value)) {
            free(key);
            fisics_json_free(value);
            fisics_json_free(object);
            return NULL;
        }
        skip_whitespace(parser);
        if (parser->position < parser->length && parser->text[parser->position] == '}') {
            parser->position++;
            return object;
        }
        if (parser->position >= parser->length || parser->text[parser->position++] != ',') {
            json_error(parser, "expected ',' or '}' in object");
            fisics_json_free(object);
            return NULL;
        }
        skip_whitespace(parser);
    }
    json_error(parser, "unterminated object");
    fisics_json_free(object);
    return NULL;
}

static FisicsJsonValue* parse_number(JsonParser* parser) {
    size_t start = parser->position;
    if (parser->text[parser->position] == '-') parser->position++;
    if (parser->position >= parser->length) return NULL;
    if (parser->text[parser->position] == '0') {
        parser->position++;
        if (parser->position < parser->length &&
            isdigit((unsigned char)parser->text[parser->position])) {
            json_error(parser, "leading zero in number");
            return NULL;
        }
    } else if (parser->text[parser->position] >= '1' &&
               parser->text[parser->position] <= '9') {
        while (parser->position < parser->length &&
               isdigit((unsigned char)parser->text[parser->position])) {
            parser->position++;
        }
    } else {
        json_error(parser, "invalid number");
        return NULL;
    }
    if (parser->position < parser->length && parser->text[parser->position] == '.') {
        parser->position++;
        if (parser->position >= parser->length ||
            !isdigit((unsigned char)parser->text[parser->position])) {
            json_error(parser, "invalid number fraction");
            return NULL;
        }
        while (parser->position < parser->length &&
               isdigit((unsigned char)parser->text[parser->position])) parser->position++;
    }
    if (parser->position < parser->length &&
        (parser->text[parser->position] == 'e' || parser->text[parser->position] == 'E')) {
        parser->position++;
        if (parser->position < parser->length &&
            (parser->text[parser->position] == '+' || parser->text[parser->position] == '-')) {
            parser->position++;
        }
        if (parser->position >= parser->length ||
            !isdigit((unsigned char)parser->text[parser->position])) {
            json_error(parser, "invalid number exponent");
            return NULL;
        }
        while (parser->position < parser->length &&
               isdigit((unsigned char)parser->text[parser->position])) parser->position++;
    }
    FisicsJsonValue* value = new_value(parser, FISICS_JSON_NUMBER);
    if (!value) return NULL;
    size_t length = parser->position - start;
    value->as.text = (char*)malloc(length + 1u);
    if (!value->as.text) {
        fisics_json_free(value);
        json_error(parser, "out of memory while parsing number");
        return NULL;
    }
    memcpy(value->as.text, parser->text + start, length);
    value->as.text[length] = '\0';
    return value;
}

static bool consume_literal(JsonParser* parser, const char* literal) {
    size_t length = strlen(literal);
    if (parser->length - parser->position < length ||
        memcmp(parser->text + parser->position, literal, length) != 0) return false;
    parser->position += length;
    return true;
}

static FisicsJsonValue* parse_value(JsonParser* parser) {
    skip_whitespace(parser);
    if (parser->position >= parser->length) {
        json_error(parser, "expected value");
        return NULL;
    }
    if (++parser->depth > FISICS_JSON_MAX_DEPTH) {
        json_error(parser, "nesting exceeds %u", FISICS_JSON_MAX_DEPTH);
        parser->depth--;
        return NULL;
    }
    char c = parser->text[parser->position];
    FisicsJsonValue* value = NULL;
    if (c == '{') {
        value = parse_object(parser);
    } else if (c == '[') {
        value = parse_array(parser);
    } else if (c == '"') {
        value = new_value(parser, FISICS_JSON_STRING);
        if (value) {
            value->as.text = parse_string(parser);
            if (!value->as.text) {
                fisics_json_free(value);
                value = NULL;
            }
        }
    } else if (c == '-' || (c >= '0' && c <= '9')) {
        value = parse_number(parser);
    } else if (consume_literal(parser, "true")) {
        value = new_value(parser, FISICS_JSON_BOOL);
        if (value) value->as.boolean = true;
    } else if (consume_literal(parser, "false")) {
        value = new_value(parser, FISICS_JSON_BOOL);
    } else if (consume_literal(parser, "null")) {
        value = new_value(parser, FISICS_JSON_NULL);
    } else {
        json_error(parser, "invalid value");
    }
    parser->depth--;
    return value;
}

bool fisics_json_parse(const char* text,
                       size_t length,
                       FisicsJsonValue** out_value,
                       FisicsJsonDiagnostic* diagnostic) {
    if (diagnostic) diagnostic->message[0] = '\0';
    if (out_value) *out_value = NULL;
    if (!text || !out_value) return false;
    JsonParser parser = {
        .text = text,
        .length = length,
        .position = 0,
        .depth = 0,
        .diagnostic = diagnostic
    };
    FisicsJsonValue* value = parse_value(&parser);
    if (!value) return false;
    skip_whitespace(&parser);
    if (parser.position != parser.length) {
        json_error(&parser, "trailing bytes after document");
        fisics_json_free(value);
        return false;
    }
    *out_value = value;
    return true;
}

const FisicsJsonValue* fisics_json_object_get(const FisicsJsonValue* object,
                                               const char* key) {
    if (!object || object->type != FISICS_JSON_OBJECT || !key) return NULL;
    for (size_t i = 0; i < object->as.object.count; ++i) {
        if (strcmp(object->as.object.members[i].key, key) == 0) {
            return object->as.object.members[i].value;
        }
    }
    return NULL;
}

bool fisics_json_integer_u64(const FisicsJsonValue* value, uint64_t* out) {
    if (!value || value->type != FISICS_JSON_NUMBER || !value->as.text || !out ||
        value->as.text[0] == '-' || strchr(value->as.text, '.') ||
        strchr(value->as.text, 'e') || strchr(value->as.text, 'E')) return false;
    errno = 0;
    char* end = NULL;
    unsigned long long parsed = strtoull(value->as.text, &end, 10);
    if (errno || !end || *end != '\0') return false;
    *out = (uint64_t)parsed;
    return true;
}

bool fisics_json_integer_size(const FisicsJsonValue* value, size_t* out) {
    uint64_t parsed = 0;
    if (!fisics_json_integer_u64(value, &parsed) || parsed > SIZE_MAX) return false;
    *out = (size_t)parsed;
    return true;
}

bool fisics_json_integer_int(const FisicsJsonValue* value, int* out) {
    if (!value || value->type != FISICS_JSON_NUMBER || !value->as.text || !out ||
        strchr(value->as.text, '.') || strchr(value->as.text, 'e') ||
        strchr(value->as.text, 'E')) return false;
    errno = 0;
    char* end = NULL;
    long parsed = strtol(value->as.text, &end, 10);
    if (errno || !end || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) return false;
    *out = (int)parsed;
    return true;
}
