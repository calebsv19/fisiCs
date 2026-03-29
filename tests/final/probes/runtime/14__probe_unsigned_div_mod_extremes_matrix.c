#include <stdio.h>

int main(void) {
    unsigned int ui = 0xffffffffu;
    unsigned int q1 = ui / 3u;
    unsigned int r1 = ui % 3u;

    unsigned long long ull = 0xffffffffffffffffull;
    unsigned long long q2 = ull / 10ull;
    unsigned long long r2 = ull % 10ull;

    printf("%u %u %llu %llu\n", q1, r1, q2, r2);
    return 0;
}
