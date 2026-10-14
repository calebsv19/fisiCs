// SPDX-License-Identifier: Apache-2.0

#include "main_internal.h"

#include <stdint.h>
#include <stdlib.h>

static size_t cross_tu_bucket(const char* name, SymbolKind kind, size_t count) {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (const unsigned char* p = (const unsigned char*)name; *p; ++p) {
        hash ^= *p;
        hash *= UINT64_C(1099511628211);
    }
    hash ^= (uint64_t)kind;
    hash *= UINT64_C(1099511628211);
    return (size_t)(hash & (count - 1u));
}

static void cross_tu_index_link(CrossTUVarDef* items,
                                CrossTUIndexBucket* buckets,
                                size_t bucketCount,
                                size_t entry) {
    CrossTUVarDef* item = &items[entry];
    CrossTUIndexBucket* bucket =
        &buckets[cross_tu_bucket(item->name, item->kind, bucketCount)];
    item->nextInBucket = CROSS_TU_INDEX_NONE;
    if (bucket->tail == CROSS_TU_INDEX_NONE) {
        bucket->head = entry;
    } else {
        items[bucket->tail].nextInBucket = entry;
    }
    bucket->tail = entry;
}

bool main_cross_tu_index_reserve(CrossTUVarDefList* defs) {
    if (!defs || defs->count == SIZE_MAX) return false;
    if (defs->bucketCount && defs->count < defs->bucketCount) return true;
    size_t count = defs->bucketCount ? defs->bucketCount : 64u;
    if (defs->bucketCount) {
        if (count > SIZE_MAX / 2u) return false;
        count *= 2u;
    }
    if (count > SIZE_MAX / sizeof(CrossTUIndexBucket)) return false;
    CrossTUIndexBucket* buckets = malloc(count * sizeof(*buckets));
    if (!buckets) return false;
    for (size_t i = 0; i < count; ++i) {
        buckets[i].head = CROSS_TU_INDEX_NONE;
        buckets[i].tail = CROSS_TU_INDEX_NONE;
    }
    /* Allocation is the only fallible step. Rebuild in ledger order so that
       the first conflicting prior declaration and its source remain stable. */
    for (size_t i = 0; i < defs->count; ++i) {
        cross_tu_index_link(defs->items, buckets, count, i);
    }
    free(defs->buckets);
    defs->buckets = buckets;
    defs->bucketCount = count;
    return true;
}

void main_cross_tu_index_append(CrossTUVarDefList* defs, size_t entry) {
    cross_tu_index_link(defs->items, defs->buckets, defs->bucketCount, entry);
}

size_t main_cross_tu_index_first(const CrossTUVarDefList* defs,
                               const char* name, SymbolKind kind) {
    if (!defs || !defs->bucketCount || !name) return CROSS_TU_INDEX_NONE;
    /* The caller retains exact name/kind checks: a hash collision is never
       evidence that two declarations describe the same symbol. */
    return defs->buckets[cross_tu_bucket(name, kind, defs->bucketCount)].head;
}

void main_cross_tu_index_free(CrossTUVarDefList* defs) {
    if (!defs) return;
    free(defs->buckets);
    defs->buckets = NULL;
    defs->bucketCount = 0;
}
