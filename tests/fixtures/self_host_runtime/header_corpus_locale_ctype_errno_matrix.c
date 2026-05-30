#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    const char *raw = "AbC-19";
    char folded[16];
    struct lconv *conv = 0;
    char *tail = 0;
    char *overflow_tail = 0;
    long parsed = 0;
    long clipped = 0;
    int letters = 0;
    int digits = 0;
    int upper = 0;
    int range_errno = 0;
    size_t i = 0;

    if (!setlocale(LC_ALL, "C")) {
        return 1;
    }
    conv = localeconv();
    if (!conv || !conv->decimal_point || strcmp(conv->decimal_point, ".") != 0) {
        return 2;
    }

    while (raw[i] != '\0') {
        unsigned char ch = (unsigned char)raw[i];
        if (isalpha(ch)) {
            letters++;
        }
        if (isdigit(ch)) {
            digits++;
        }
        if (isupper(ch)) {
            upper++;
        }
        folded[i] = (char)tolower(ch);
        i++;
    }
    folded[i] = '\0';

    errno = 0;
    parsed = strtol("-42x", &tail, 10);
    if (!tail || *tail != 'x' || parsed != -42L || strcmp(folded, "abc-19") != 0) {
        return 3;
    }

    errno = 0;
    clipped = strtol("999999999999999999999999999999999999", &overflow_tail, 10);
    range_errno = errno;
    if (!overflow_tail || *overflow_tail != '\0') {
        return 4;
    }
    if (clipped != LONG_MAX || range_errno != ERANGE) {
        return 5;
    }
    if (letters != 3 || digits != 2 || upper != 2) {
        return 6;
    }

    printf(
        "folded=%s letters=%d digits=%d upper=%d value=%ld range=%d max=%ld\n",
        folded,
        letters,
        digits,
        upper,
        parsed,
        range_errno,
        (long)LONG_MAX);
    return 0;
}
