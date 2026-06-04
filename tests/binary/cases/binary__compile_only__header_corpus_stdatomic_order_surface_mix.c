#include <stdatomic.h>

int header_corpus_wave13_apply_orders(atomic_int *value, memory_order first, memory_order second) {
    int loaded = 0;

    if (!value) {
        return 0;
    }

    loaded = atomic_load_explicit(value, first);
    atomic_store_explicit(value, loaded + 1, second);
    return loaded;
}
