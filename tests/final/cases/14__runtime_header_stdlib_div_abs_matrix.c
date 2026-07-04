#include <stdio.h>
#include <stdlib.h>

int main(void) {
    div_t d = div(-17, 5);
    ldiv_t ld = ldiv(37L, 6L);
    lldiv_t lld = lldiv(-123LL, 17LL);
    int a = abs(-9);
    long b = labs(-42L);
    long long c = llabs(-111LL);
    long summary = (long)a + b + (long)c + d.quot + d.rem + ld.quot + ld.rem +
                   (long)lld.quot + (long)lld.rem;

    printf("stdlib-divabs d=%d,%d ld=%ld,%ld lld=%lld,%lld abs=%d/%ld/%lld summary=%ld\n",
           d.quot,
           d.rem,
           ld.quot,
           ld.rem,
           lld.quot,
           lld.rem,
           a,
           b,
           c,
           summary);

    return d.quot == -3 && d.rem == -2 && ld.quot == 6L && ld.rem == 1L &&
                   lld.quot == -7LL && lld.rem == -4LL && a == 9 &&
                   b == 42L && c == 111LL && summary == 153L
               ? 0
               : 1;
}
