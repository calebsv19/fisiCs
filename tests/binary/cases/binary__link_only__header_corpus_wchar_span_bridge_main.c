#include <wchar.h>

int header_corpus_wave11_span_score(const wchar_t *src);

int main(void) {
    return header_corpus_wave11_span_score(L"wave11") == 53 ? 0 : 1;
}
