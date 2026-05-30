#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <stdlib.h>

int header_corpus_errno_digit_score(const char* raw) {
    char* end = 0;
    long value = 0;
    int score = 0;

    errno = 0;
    value = strtol(raw, &end, 10);

    if (raw && isdigit((unsigned char)raw[0])) {
        score += 100;
    }
    if (end && *end == '\0') {
        score += 10;
    }
    if (errno == 0) {
        score += 1;
    }
    if (DBL_EPSILON > 0.0) {
        score += 1;
    }

    return score + (int)value;
}
