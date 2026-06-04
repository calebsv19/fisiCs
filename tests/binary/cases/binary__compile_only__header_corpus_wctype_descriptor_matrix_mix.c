#include <wchar.h>
#include <wctype.h>

static int header_corpus_wave35_wctype_descriptor_surface(wint_t value) {
    wctype_t alpha = wctype("alpha");
    wctype_t xdigit = wctype("xdigit");
    wctrans_t upper = wctrans("toupper");
    int score = 0;
    score += iswctype(value, alpha) ? 5 : 0;
    score += iswctype(value, xdigit) ? 7 : 0;
    score += (int)towctrans(value, upper);
    score += (int)towlower(L'Q');
    return score;
}

int main(void) {
    return header_corpus_wave35_wctype_descriptor_surface(L'a') == 0;
}
