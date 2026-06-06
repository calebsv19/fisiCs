#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int first_a = 0;
    int first_b = 0;
    int first_c = 0;
    int repeat_a = 0;
    int repeat_b = 0;
    int repeat_c = 0;
    int other_a = 0;
    int other_b = 0;
    long summary = 0;
    int repeat_ok = 0;
    int range_ok = 0;
    int other_diff = 0;

    srand(12345u);
    first_a = rand();
    first_b = rand();
    first_c = rand();

    srand(12345u);
    repeat_a = rand();
    repeat_b = rand();
    repeat_c = rand();

    srand(7u);
    other_a = rand();
    other_b = rand();

    repeat_ok = first_a == repeat_a && first_b == repeat_b && first_c == repeat_c;
    range_ok = first_a >= 0 && first_a <= RAND_MAX && first_b >= 0 &&
               first_b <= RAND_MAX && first_c >= 0 && first_c <= RAND_MAX &&
               other_a >= 0 && other_a <= RAND_MAX && other_b >= 0 &&
               other_b <= RAND_MAX;
    other_diff = other_a != first_a || other_b != first_b;
    summary = (long)(first_a % 997) + (long)(first_b % 991) +
              (long)(first_c % 983) + (long)(other_a % 977) +
              (long)(other_b % 971) + repeat_ok * 17L + range_ok * 19L +
              other_diff * 23L;

    printf("stdlib-rand-srand max=%d repeat=%d range=%d other=%d first=%d/%d/%d otherseq=%d/%d summary=%ld\n",
           RAND_MAX,
           repeat_ok,
           range_ok,
           other_diff,
           first_a,
           first_b,
           first_c,
           other_a,
           other_b,
           summary);

    return repeat_ok && range_ok && other_diff ? 0 : 1;
}
