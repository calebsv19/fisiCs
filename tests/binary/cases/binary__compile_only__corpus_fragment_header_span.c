#include "binary__corpus_fragment_header_span_ops.h"

static int slice_eq(Slice a, Slice b) {
    size_t i = 0;
    if (a.len != b.len) return 0;
    while (i < a.len) {
        if (a.ptr[i] != b.ptr[i]) return 0;
        ++i;
    }
    return 1;
}

unsigned entry_lookup(const Entry *table, size_t count, Slice key) {
    size_t i = 0;
    while (i < count) {
        if (!ENTRY_IS_EMPTY(table[i]) &&
            ENTRY_MATCH_LEN(table[i], key.len) &&
            slice_eq(table[i].key, key)) {
            return table[i].value;
        }
        ++i;
    }
    return 0u;
}
