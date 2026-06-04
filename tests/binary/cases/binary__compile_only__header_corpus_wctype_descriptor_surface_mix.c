#include <wchar.h>
#include <wctype.h>

static wint_t header_corpus_wave11_fold(wint_t ch) {
    wctrans_t lower = wctrans("tolower");
    wctype_t alpha = wctype("alpha");
    return iswctype(ch, alpha) ? towctrans(ch, lower) : ch;
}

int main(void) {
    wchar_t sample[8] = L"Ab";
    mbstate_t state = {0};
    sample[0] = (wchar_t)header_corpus_wave11_fold(sample[0]);
    return (int)sample[0] + (int)sizeof(state);
}
