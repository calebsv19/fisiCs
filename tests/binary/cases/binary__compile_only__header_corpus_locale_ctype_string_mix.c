#include <ctype.h>
#include <locale.h>
#include <stddef.h>
#include <string.h>

struct HeaderCorpusLocaleFold {
    char text[16];
    size_t len;
};

static size_t fold_ascii_letters(const char *src, char *dst, size_t cap) {
    size_t i = 0;

    if (!src || !dst || cap == 0u) {
        return 0u;
    }

    while (src[i] != '\0' && i + 1u < cap) {
        dst[i] = (char)tolower((unsigned char)src[i]);
        i++;
    }
    dst[i] = '\0';
    return i;
}

int header_corpus_locale_ctype_string_mix(struct HeaderCorpusLocaleFold *out, const char *src) {
    struct lconv *conv = localeconv();
    size_t len = 0;

    if (!out || !src || !conv || !conv->decimal_point) {
        return 0;
    }

    len = fold_ascii_letters(src, out->text, sizeof(out->text));
    out->len = len;
    return isalpha((unsigned char)src[0]) && strchr(out->text, '-') != 0
               && strcmp(conv->decimal_point, ".") != 0;
}
