#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char *tail_a = 0;
    char *tail_b = 0;
    intmax_t signed_value = strtoimax("-7fff!", &tail_a, 16);
    uintmax_t unsigned_value = strtoumax("755?", &tail_b, 8);
    int ok = tail_a && *tail_a == '!' && tail_b && *tail_b == '?';

    printf("inttypes-scan signed=%" PRIdMAX " unsigned=%" PRIuMAX " tails=%c/%c ok=%d\n",
           signed_value,
           unsigned_value,
           tail_a ? *tail_a : '?',
           tail_b ? *tail_b : '?',
           ok);
    return signed_value == INTMAX_C(-32767) && unsigned_value == UINTMAX_C(493) && ok == 1 ? 0 : 1;
}
