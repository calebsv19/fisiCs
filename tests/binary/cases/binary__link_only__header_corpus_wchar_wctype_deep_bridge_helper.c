#include <wchar.h>
#include <wctype.h>

int header_corpus_wave35_wchar_wctype_score(const wchar_t *value) {
    wctype_t alpha = wctype("alpha");
    wchar_t local[16];
    const wchar_t *hit;
    size_t len;
    wmemset(local, L'_', 15);
    local[15] = L'\0';
    wmemcpy(local, value, wcslen(value) + 1);
    hit = wcspbrk(local, L"-_");
    len = wcsspn(local, L"wide");
    return (int)len + (hit ? (int)(hit - local) : -1) +
           (iswctype(local[0], alpha) ? 10 : 0);
}
