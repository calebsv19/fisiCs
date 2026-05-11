// SPDX-License-Identifier: Apache-2.0

#include "keyword_lookup.h"

#include <string.h>

typedef struct KeywordEntry {
    const char* name;
    size_t len;
} KeywordEntry;

static const KeywordEntry kKeywords[] = {
    {"int", 3},
    {"char", 4},
    {"float", 5},
    {"double", 6},
    {"long", 4},
    {"short", 5},
    {"signed", 6},
    {"unsigned", 8},
    {"enum", 4},
    {"union", 5},
    {"struct", 6},
    {"typedef", 7},
    {"void", 4},
    {"if", 2},
    {"else", 4},
    {"for", 3},
    {"while", 5},
    {"do", 2},
    {"switch", 6},
    {"case", 4},
    {"default", 7},
    {"return", 6},
    {"goto", 4},
    {"break", 5},
    {"continue", 8},
    {"extern", 6},
    {"static", 6},
    {"auto", 4},
    {"register", 8},
    {"_Thread_local", 13},
    {"const", 5},
    {"volatile", 8},
    {"restrict", 8},
    {"inline", 6},
    {"_Atomic", 7},
    {"sizeof", 6},
    {"_Alignof", 8},
    {"alignof", 7},
    {"_Static_assert", 14},
    {"_Bool", 5},
    {"asm", 3},
    {"_Complex", 8},
    {"_Imaginary", 10},
    {"pragma", 6},
    {"once", 4},
};

const char* in_keyword_set(const char* str, size_t len) {
    if (!str) {
        return NULL;
    }

    for (size_t i = 0; i < (sizeof(kKeywords) / sizeof(kKeywords[0])); ++i) {
        const KeywordEntry* entry = &kKeywords[i];
        if (entry->len != len) {
            continue;
        }
        if (memcmp(str, entry->name, len) == 0) {
            return entry->name;
        }
    }

    return NULL;
}
