#ifndef BINARY_CORPUS_FRAGMENT_HEADER_SPAN_TYPES_H
#define BINARY_CORPUS_FRAGMENT_HEADER_SPAN_TYPES_H

#include <stddef.h>

typedef struct {
    const char *ptr;
    size_t len;
} Slice;

typedef struct {
    Slice key;
    unsigned value;
} Entry;

#endif
