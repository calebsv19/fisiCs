#include <limits.h>
#include <stdio.h>

int main(void) {
    int checks = 0;
    int char_bits = CHAR_BIT;
    int short_bits = (int)(sizeof(short) * CHAR_BIT);
    int int_bits = (int)(sizeof(int) * CHAR_BIT);
    int long_bits = (int)(sizeof(long) * CHAR_BIT);
    int llong_bits = (int)(sizeof(long long) * CHAR_BIT);

    checks += (SCHAR_MIN + SCHAR_MAX) == -1;
    checks += (SHRT_MIN + SHRT_MAX) == -1;
    checks += (INT_MIN + INT_MAX) == -1;
    checks += (LONG_MIN + LONG_MAX) == -1L;
    checks += (LLONG_MIN + LLONG_MAX) == -1LL;

    printf("limits-signed checks=%d widths=%d/%d/%d/%d/%d\n",
           checks,
           char_bits,
           short_bits,
           int_bits,
           long_bits,
           llong_bits);
    return checks == 5 ? 0 : 1;
}
