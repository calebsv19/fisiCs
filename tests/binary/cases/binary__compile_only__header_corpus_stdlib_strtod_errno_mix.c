#include <errno.h>
#include <float.h>
#include <stdbool.h>
#include <stdlib.h>

static bool header_corpus_wave8_prepare_parse(
    const char *text,
    double fallback,
    double *value_out,
    char *tail_out
) {
    char *tail = 0;
    double value = 0.0;

    if (!text || !value_out || !tail_out) {
        return false;
    }

    errno = 0;
    value = strtod(text, &tail);
    if (!tail || errno != 0) {
        value = fallback;
    }

    *value_out = value;
    *tail_out = tail ? *tail : '\0';
    return value <= DBL_MAX;
}

int main(void) {
    double value = 0.0;
    char tail = '\0';

    return header_corpus_wave8_prepare_parse("4.5x", 0.0, &value, &tail) ? 0 : 1;
}
