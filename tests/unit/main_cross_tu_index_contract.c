// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main_internal.h"

/* Exercise the production implementation with deterministic allocator failure,
   without adding a runtime testing switch to the compiler. */
static int reject_allocation;
static void* index_test_malloc(size_t size) {
    return reject_allocation ? NULL : malloc(size);
}
#define malloc index_test_malloc
#include "../../src/main_cross_tu_index.c"
#undef malloc
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

int main(void) {
    CrossTUVarDefList defs = {0};
    CHECK(main_cross_tu_index_first(&defs, "same", SYMBOL_VARIABLE) == CROSS_TU_INDEX_NONE);
    reject_allocation = 1;
    CHECK(!main_cross_tu_index_reserve(&defs));
    CHECK(!defs.buckets && !defs.bucketCount);
    main_cross_tu_index_free(&defs); /* Failed first insertion can have no items. */
    reject_allocation = 0;
    char names[300][32];
    for (size_t i = 0; i < 300; ++i) {
        if (i == 64 || i == 128 || i == 256) {
            CrossTUIndexBucket* before = defs.buckets;
            size_t links[300];
            for (size_t j = 0; j < defs.count; ++j) links[j] = defs.items[j].nextInBucket;
            reject_allocation = 1;
            CHECK(!main_cross_tu_index_reserve(&defs));
            CHECK(defs.buckets == before && defs.bucketCount == i);
            for (size_t j = 0; j < defs.count; ++j) CHECK(links[j] == defs.items[j].nextInBucket);
            reject_allocation = 0;
        }
        CHECK(main_cross_tu_index_reserve(&defs));
        /* Move the backing vector on every append: the index must not borrow it. */
        CrossTUVarDef* grown = calloc(i + 1, sizeof(*grown));
        CHECK(grown);
        if (i) memcpy(grown, defs.items, i * sizeof(*grown));
        free(defs.items); defs.items = grown;
        snprintf(names[i], sizeof(names[i]), "name_%zu", i);
        defs.items[i].name = i % 3 ? names[i] : "same";
        defs.items[i].kind = i % 2 ? SYMBOL_FUNCTION : SYMBOL_VARIABLE;
        main_cross_tu_index_append(&defs, i); defs.count++;
        /* Compare ordered candidate selection against an independent linear oracle. */
        for (size_t q = 0; q <= i; ++q) {
            size_t actual = main_cross_tu_index_first(&defs, defs.items[q].name, defs.items[q].kind);
            for (size_t expected = 0; expected <= i; ++expected) {
                if (defs.items[expected].kind != defs.items[q].kind ||
                    strcmp(defs.items[expected].name, defs.items[q].name)) continue;
                while (actual != CROSS_TU_INDEX_NONE &&
                       (defs.items[actual].kind != defs.items[q].kind ||
                        strcmp(defs.items[actual].name, defs.items[q].name)))
                    actual = defs.items[actual].nextInBucket;
                CHECK(actual == expected);
                actual = defs.items[actual].nextInBucket;
            }
        }
    }
    /* Real unequal-name collisions occurred and were filtered by the oracle. */
    int collision = 0;
    for (size_t i = 0; i < defs.count; ++i) {
        size_t next = defs.items[i].nextInBucket;
        if (next != CROSS_TU_INDEX_NONE && strcmp(defs.items[i].name, defs.items[next].name)) collision = 1;
    }
    CHECK(collision);
    main_cross_tu_index_free(&defs); main_cross_tu_index_free(&defs);
    CHECK(!defs.buckets && !defs.bucketCount);
    free(defs.items);
    puts("cross-TU index contract: PASS"); return 0;
}
