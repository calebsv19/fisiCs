#include <limits.h>
#include <stdio.h>

int main(void) {
    unsigned long score = 0;

    score += (unsigned long)CHAR_BIT;
    score += (unsigned long)(UCHAR_MAX & 0xffu);
    score += (unsigned long)(USHRT_MAX & 0xffu);
    score += (unsigned long)(UINT_MAX & 0xffu);
    score += (LONG_MAX > INT_MAX) ? 100ul : 0ul;
    score += (LLONG_MAX >= LONG_MAX) ? 200ul : 0ul;

    printf("limits-unsigned score=%lu\n", score);
    return score == 1073ul ? 0 : 1;
}
