#include <locale.h>

static int wave32_locale_lconv_surface(void) {
    struct lconv *conv = localeconv();
    const char *prior = setlocale(LC_ALL, "C");
    const char *numeric = setlocale(LC_NUMERIC, 0);
    const char *time_name = setlocale(LC_TIME, 0);
    int score = 0;

    if (prior) {
        score++;
    }
    if (numeric) {
        score++;
    }
    if (time_name) {
        score++;
    }
    if (conv && conv->decimal_point && conv->thousands_sep) {
        score++;
    }
    if (conv && conv->currency_symbol && conv->positive_sign && conv->negative_sign) {
        score++;
    }

    return score == 5 ? 0 : 1;
}

int main(void) {
    return wave32_locale_lconv_surface();
}
