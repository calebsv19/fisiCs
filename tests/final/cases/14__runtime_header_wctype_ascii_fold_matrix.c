#include <wchar.h>
#include <wctype.h>
#include <stdio.h>

int main(void) {
    wint_t upper = towupper(btowc('a'));
    wint_t lower = towlower(L'Q');
    wctrans_t updesc = wctrans("toupper");
    wctype_t alpha = wctype("alpha");
    wint_t trans = towctrans(L'm', updesc);
    printf("alpha=%d digit=%d upper=%d lower=%d trans=%d back=%d\n",
           iswctype(L'A', alpha) ? 1 : 0,
           iswdigit(L'7') ? 1 : 0,
           (int)upper,
           (int)lower,
           (int)trans,
           (int)wctob(L'Z'));
    return 0;
}
