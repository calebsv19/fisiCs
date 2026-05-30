#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

uintmax_t header_corpus_wave6_fold_bytes(const uint8_t *bytes, size_t count) {
    size_t i = 0;
    uintmax_t acc = UINTMAX_C(0);

    for (i = 0; i < count; ++i) {
        acc = acc * UINTMAX_C(257) + (uintmax_t)bytes[i];
    }

    return acc;
}
