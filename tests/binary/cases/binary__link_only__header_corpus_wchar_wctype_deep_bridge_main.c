#include <wchar.h>

int header_corpus_wave35_wchar_wctype_score(const wchar_t *value);

int main(void) {
    return header_corpus_wave35_wchar_wctype_score(L"wide-zone") == 14 ? 0 : 1;
}
