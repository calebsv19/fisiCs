#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char *big_text = "999999999999999999999999999999!";
    const char *neg_text = "-999999999999999999999999999999?";
    const char *wide_text = "184467440737095516160tail";
    const char *partial_text = "  +77xyz";
    char *tail_big = 0;
    char *tail_neg = 0;
    char *tail_wide = 0;
    char *tail_partial = 0;

    errno = 0;
    long big = strtol(big_text, &tail_big, 10);
    int e_big = errno == ERANGE;

    errno = 0;
    long neg = strtol(neg_text, &tail_neg, 10);
    int e_neg = errno == ERANGE;

    errno = 0;
    unsigned long long wide = strtoull(wide_text, &tail_wide, 10);
    int e_wide = errno == ERANGE;

    errno = 0;
    long partial = strtol(partial_text, &tail_partial, 10);
    int e_partial = errno;

    long off_big = (long)(tail_big - big_text);
    long off_neg = (long)(tail_neg - neg_text);
    long off_wide = (long)(tail_wide - wide_text);
    long off_partial = (long)(tail_partial - partial_text);
    unsigned long long low = wide & 255ULL;
    long summary = off_big + off_neg + off_wide + off_partial + partial + e_big + e_neg + e_wide + e_partial;

    printf("stdlib-strto-overflow big=%d off=%ld neg=%d off=%ld wide=%d off=%ld low=%llu partial=%ld off=%ld errno=%d summary=%ld\n",
           big == LONG_MAX,
           off_big,
           neg == LONG_MIN,
           off_neg,
           wide == ULLONG_MAX,
           off_wide,
           low,
           partial,
           off_partial,
           e_partial,
           summary);

    return big == LONG_MAX && off_big == 30L && e_big &&
                   neg == LONG_MIN && off_neg == 31L && e_neg &&
                   wide == ULLONG_MAX && off_wide == 21L && e_wide &&
                   low == 255ULL && partial == 77L && off_partial == 5L &&
                   e_partial == 0 && summary == 167L
               ? 0
               : 1;
}
