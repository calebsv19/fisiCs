#include <stdio.h>

int main(void) {
    int values[8] = {3, 5, 7, 11, 13, 17, 19, 23};
    int* p = &values[5];

    int a = p[-2];
    int b = *(p - 5);
    long long c = (long long)(p - &values[1]);
    int d = (int)((p - 3) - values);

    printf("%d %d %lld %d\n", a, b, c, d);
    return 0;
}
