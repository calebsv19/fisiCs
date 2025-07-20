#ifndef KEYWORD_LOOKUP_H
#define KEYWORD_LOOKUP_H

#include <stddef.h>

struct keyword {
    const char *name;
    int token;
};

// Provided by gperf
const struct keyword *in_keyword_set(const char *str, size_t len);

#endif

