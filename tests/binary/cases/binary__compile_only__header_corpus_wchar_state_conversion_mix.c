#include <wchar.h>

static int header_corpus_wave11_stateful_roundtrip(const char *src) {
    mbstate_t in_state = {0};
    mbstate_t out_state = {0};
    wchar_t wide = 0;
    char outbuf[8];
    size_t used = mbrtowc(&wide, src, 1, &in_state);
    if (used == (size_t)-1 || used == (size_t)-2) {
        return -1;
    }
    if (wcrtomb(outbuf, wide, &out_state) == (size_t)-1) {
        return -2;
    }
    return mbsinit(&in_state) ? (int)wide : -3;
}

int main(void) {
    return header_corpus_wave11_stateful_roundtrip("A") == 'A' ? 0 : 1;
}
