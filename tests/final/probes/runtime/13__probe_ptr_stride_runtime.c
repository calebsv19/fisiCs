#include <stdio.h>

int main(void) {
    long long values[6] = {2, 4, 6, 8, 10, 12};
    long long *p = &values[1];
    long long *q = &values[5];

    printf("%lld %lld %lld\n", (long long)(q - p), *(p + 2), *(q - 3));
    return 0;
}
