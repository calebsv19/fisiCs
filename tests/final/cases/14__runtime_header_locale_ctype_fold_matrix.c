#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *raw = "AbC-19";
    char folded[16];
    struct lconv *conv = 0;
    int letters = 0;
    int digits = 0;
    int upper = 0;
    size_t i = 0;

    if (!setlocale(LC_ALL, "C")) {
        return 1;
    }

    conv = localeconv();
    if (!conv || !conv->decimal_point) {
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

    if (strcmp(folded, "abc-19") != 0) {
        return 3;
    }
    if (letters != 3 || digits != 2 || upper != 2 || strcmp(conv->decimal_point, ".") != 0) {
        return 4;
    }

    printf(
        "folded=%s letters=%d digits=%d upper=%d decimal=%s\n",
        folded,
        letters,
        digits,
        upper,
        conv->decimal_point);
    return 0;
}
