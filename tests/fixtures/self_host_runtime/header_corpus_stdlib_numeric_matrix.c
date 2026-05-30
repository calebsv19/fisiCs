#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static bool header_corpus_wave8_parse_and_divide(
    const char *first_text,
    const char *second_text,
    double *first_value_out,
    char *first_tail_out,
    double *second_value_out,
    char *second_tail_out,
    div_t *first_div_out,
    ldiv_t *second_div_out,
    int *abs_out,
    long *labs_out
) {
    char *first_tail = 0;
    char *second_tail = 0;
    double first_value = 0.0;
    double second_value = 0.0;
    div_t first_div;
    ldiv_t second_div;
    int abs_value = 0;
    long labs_value = 0L;

    if (!first_text || !second_text || !first_value_out || !first_tail_out ||
        !second_value_out || !second_tail_out || !first_div_out ||
        !second_div_out || !abs_out || !labs_out) {
        return false;
    }

    first_value = strtod(first_text, &first_tail);
    second_value = strtod(second_text, &second_tail);
    first_div = div(29, 5);
    second_div = ldiv(-17L, 4L);
    abs_value = abs(-17);
    labs_value = labs(-17L);

    if (!first_tail || !second_tail) {
        return false;
    }

    *first_value_out = first_value;
    *first_tail_out = *first_tail;
    *second_value_out = second_value;
    *second_tail_out = *second_tail;
    *first_div_out = first_div;
    *second_div_out = second_div;
    *abs_out = abs_value;
    *labs_out = labs_value;
    return true;
}

int main(void) {
    double first_value = 0.0;
    double second_value = 0.0;
    char first_tail = '\0';
    char second_tail = '\0';
    div_t first_div;
    ldiv_t second_div;
    int abs_value = 0;
    long labs_value = 0L;
    bool ok = false;

    ok = header_corpus_wave8_parse_and_divide(
        "12.5x",
        "-0.125?",
        &first_value,
        &first_tail,
        &second_value,
        &second_tail,
        &first_div,
        &second_div,
        &abs_value,
        &labs_value);

    if (!ok) {
        return 1;
    }
    if (first_value != 12.5 || first_tail != 'x') {
        return 2;
    }
    if (second_value != -0.125 || second_tail != '?') {
        return 3;
    }
    if (first_div.quot != 5 || first_div.rem != 4) {
        return 4;
    }
    if (second_div.quot != -4L || second_div.rem != -1L) {
        return 5;
    }
    if (abs_value != 17 || labs_value != 17L) {
        return 6;
    }

    printf(
        "first=%.3f tail=%c second=%.3f tail2=%c div=%d/%d ldiv=%ld/%ld abs=%d labs=%ld\n",
        first_value,
        first_tail,
        second_value,
        second_tail,
        first_div.quot,
        first_div.rem,
        second_div.quot,
        second_div.rem,
        abs_value,
        labs_value);
    return 0;
}
