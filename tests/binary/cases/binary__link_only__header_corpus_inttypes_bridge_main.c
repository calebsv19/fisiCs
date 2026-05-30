#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

uintmax_t header_corpus_wave6_fold_bytes(const uint8_t *bytes, size_t count);

int main(void) {
    static const uint8_t bytes[] = {3U, 1U, 4U, 1U, 5U};
    return header_corpus_wave6_fold_bytes(bytes, sizeof(bytes)) > UINTMAX_C(0)
               ? 0
               : 1;
}
