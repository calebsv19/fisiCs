#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool header_corpus_wave6_parse_and_divide(
    const char *dec_text,
    const char *hex_text,
    intmax_t *dec_out,
    char *dec_tail_out,
    uintmax_t *hex_out,
    char *hex_tail_out,
    imaxdiv_t *split_out,
    intmax_t *abs_out
) {
    char *dec_tail = 0;
    char *hex_tail = 0;
    intmax_t dec_value = 0;
    uintmax_t hex_value = 0;
    imaxdiv_t split;
    intmax_t magnitude = 0;

    if (!dec_text || !hex_text || !dec_out || !dec_tail_out || !hex_out ||
        !hex_tail_out || !split_out || !abs_out) {
        return false;
    }

    dec_value = strtoimax(dec_text, &dec_tail, 10);
    hex_value = strtoumax(hex_text, &hex_tail, 16);
    split = imaxdiv(INTMAX_C(29), INTMAX_C(5));
    magnitude = imaxabs(INTMAX_C(-17));

    if (!dec_tail || !hex_tail) {
        return false;
    }

    *dec_out = dec_value;
    *dec_tail_out = *dec_tail;
    *hex_out = hex_value;
    *hex_tail_out = *hex_tail;
    *split_out = split;
    *abs_out = magnitude;
    return true;
}

int main(void) {
    static const uint8_t bytes[] = {3U, 1U, 4U, 1U};
    intmax_t dec_value = 0;
    uintmax_t hex_value = 0;
    intmax_t magnitude = 0;
    char dec_tail = '\0';
    char hex_tail = '\0';
    imaxdiv_t split;
    uintptr_t span = (uintptr_t)(&bytes[3] - &bytes[0]);
    bool ok = false;

    ok = header_corpus_wave6_parse_and_divide(
        "-4096z",
        "7b!",
        &dec_value,
        &dec_tail,
        &hex_value,
        &hex_tail,
        &split,
        &magnitude);

    if (!ok) {
        return 1;
    }
    if (dec_value != INTMAX_C(-4096) || dec_tail != 'z') {
        return 2;
    }
    if (hex_value != UINTMAX_C(123) || hex_tail != '!') {
        return 3;
    }
    if (split.quot != INTMAX_C(5) || split.rem != INTMAX_C(4)) {
        return 4;
    }
    if (magnitude != INTMAX_C(17) || span != (uintptr_t)3) {
        return 5;
    }

    printf(
        "dec=%" PRIdMAX " tail=%c hex=%" PRIuMAX " htail=%c quot=%" PRIdMAX " rem=%" PRIdMAX " abs=%" PRIdMAX " span=%" PRIuPTR "\n",
        dec_value,
        dec_tail,
        hex_value,
        hex_tail,
        split.quot,
        split.rem,
        magnitude,
        span);
    return 0;
}
