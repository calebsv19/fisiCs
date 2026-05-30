#include <wchar.h>
#include <wctype.h>

static wint_t header_corpus_wave11_fold(wint_t ch) {
    if (iswlower(ch)) {
        return towupper(ch);
    }
    return towlower(ch);
}

int main(void) {
    wchar_t sample[8] = L"ab";
    wint_t first = btowc('A');
    wint_t folded = header_corpus_wave11_fold(L'q');
    return (sample[0] && first != WEOF && folded == L'Q') ? 0 : 1;
}
