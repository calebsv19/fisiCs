#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static int header_corpus_wave6_build_line(
    char *buf,
    size_t len,
    intmax_t signed_value,
    uintmax_t hex_value
) {
    return snprintf(
        buf,
        len,
        "s=%" PRIdMAX " h=%" PRIxMAX " p=%" PRIuPTR,
        signed_value,
        hex_value,
        (uintptr_t)len);
}

int main(void) {
    char buffer[96];
    return header_corpus_wave6_build_line(
               buffer,
               sizeof(buffer),
               INTMAX_C(-7),
               UINTMAX_C(0x2a)) < 0;
}
