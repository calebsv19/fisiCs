#include <wchar.h>

int header_corpus_wave11_span_score(const wchar_t *src) {
    wchar_t tmp[16];
    const wchar_t *last;
    wmemcpy(tmp, src, wcslen(src) + 1);
    last = wcsrchr(tmp, L'1');
    return (int)wcslen(tmp) * 8 + (last ? (int)(last - tmp) : -1);
}
