#include <wchar.h>

static int header_corpus_wave35_wchar_string_surface(const wchar_t *value) {
    wchar_t scratch[16] = L"wide-case";
    const wchar_t *dash = wcschr(scratch, L'-');
    const wchar_t *case_tail = wcsstr(scratch, L"case");
    size_t len = wcslen(value);
    int prefix = wcsncmp(value, L"wide", 4);
    wmemmove(scratch + 5, scratch, 4);
    return (int)len + prefix + (dash != 0) + (case_tail != 0) + (int)scratch[5];
}

int main(void) {
    return header_corpus_wave35_wchar_string_surface(L"wide-case") == 0;
}
