#ifndef BINARY_CORPUS_FRAGMENT_SYMBOL_POOL_H
#define BINARY_CORPUS_FRAGMENT_SYMBOL_POOL_H

#include <stddef.h>

typedef struct {
    const char *name;
    size_t len;
    unsigned id;
} SymEntry;

unsigned sym_hash(const char *s, size_t n);
int sym_pool_add(const char *name, size_t len);
int sym_pool_find(const char *name, size_t len);
unsigned sym_pool_count(void);

#endif
