#ifndef BINARY_CORPUS_FRAGMENT_HEADER_SPAN_OPS_H
#define BINARY_CORPUS_FRAGMENT_HEADER_SPAN_OPS_H

#include "binary__corpus_fragment_header_span_types.h"

#define ENTRY_IS_EMPTY(e) ((e).key.len == 0u)
#define ENTRY_MATCH_LEN(e, n) ((e).key.len == (n))

unsigned entry_lookup(const Entry *table, size_t count, Slice key);

#endif
