#include <inttypes.h>
#include <limits.h>
#include <stdint.h>

struct HeaderCorpusWave6Slot {
    intmax_t signed_value;
    uintmax_t unsigned_value;
    uintptr_t address_bits;
};

static int header_corpus_wave6_limits_ok(void) {
    struct HeaderCorpusWave6Slot slot = {
        INTMAX_C(-32),
        UINTMAX_C(255),
        (uintptr_t)UINTPTR_MAX,
    };
    return slot.signed_value < 0 &&
           slot.unsigned_value > 0 &&
           slot.address_bits >= (uintptr_t)0 &&
           INTMAX_MAX >= INTMAX_C(127) &&
           UCHAR_MAX >= 0xffU;
}

int main(void) {
    return header_corpus_wave6_limits_ok() ? 0 : 1;
}
