#include <wchar.h>
#include <wctype.h>
#include <stdio.h>

static int header_corpus_wave11_wide_summary(void) {
    wchar_t dst[8];
    const wchar_t src[] = L"wide";
    wchar_t *hit = 0;
    wint_t upper = 0;
    wint_t lower = 0;
    wint_t trans = 0;
    wctrans_t updesc;
    wctype_t alpha;

    wmemset(dst, L'_', 7);
    dst[7] = L'\0';
    wmemcpy(dst, src, 5);
    hit = wcschr(dst, L'd');

    updesc = wctrans("toupper");
    alpha = wctype("alpha");
    upper = towupper(btowc('a'));
    lower = towlower(L'Q');
    trans = towctrans(L'm', updesc);

    return (int)wcslen(dst) * 100 +
           (hit ? (int)(hit - dst) : -1) * 10 +
           (wcscmp(dst, L"wide") == 0 ? 1 : 0) +
           (iswctype(L'A', alpha) ? 1000 : 0) +
           (iswdigit(L'7') ? 2000 : 0) +
           (int)upper +
           (int)lower +
           (int)trans +
           (int)wctob(L'Z');
}

int main(void) {
    int summary = header_corpus_wave11_wide_summary();
    if (summary != 4324) {
        return 1;
    }

    printf("summary=%d\n", summary);
    return 0;
}
