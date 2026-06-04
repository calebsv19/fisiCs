#include <wchar.h>
#include <wctype.h>
#include <stdio.h>

int main(void) {
    wctype_t alpha = wctype("alpha");
    wctype_t xdigit = wctype("xdigit");
    wctrans_t upper = wctrans("toupper");
    wctrans_t lower = wctrans("tolower");
    wint_t up = towctrans(L'm', upper);
    wint_t down = towctrans(L'Q', lower);
    printf("wctype-desc alpha=%d xdigit=%d digit=%d up=%d down=%d null=%d\n",
           iswctype(L'F', alpha) ? 1 : 0,
           iswctype(L'F', xdigit) ? 1 : 0,
           iswctype(L'g', xdigit) ? 1 : 0,
           (int)up,
           (int)down,
           wctype("not-a-class") == (wctype_t)0 ? 1 : 0);
    return 0;
}
