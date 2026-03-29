#include <limits.h>
#include <stdio.h>

int main(void) {
    long long max_v = LLONG_MAX;
    long long min_v = LLONG_MIN;
    unsigned long long umax_v = ULLONG_MAX;

    int sign_ok = (max_v > 0) && (min_v < 0);
    long long q = max_v / 1000000ll;
    unsigned long long r = umax_v % 97ull;
    int ll_bits = (int)(sizeof(long long) * CHAR_BIT);

    printf("%d %lld %llu %d %d\n", sign_ok, q, r, CHAR_BIT, ll_bits);
    return 0;
}
