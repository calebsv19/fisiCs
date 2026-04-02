#include <limits.h>
#include <stdio.h>

int main(void) {
    int si = -1;
    unsigned int ui = 1u;
    unsigned int umax = UINT_MAX;
    unsigned long long ull = (unsigned long long)ui;

    int a = (si < ui);
    int b = (si > ui);
    int c = ((unsigned int)si == umax);
    int d = ((long long)-2 < ull);
    int e = ((unsigned int)0 - 1u == umax);

    printf("%d %d %d %d %d\n", a, b, c, d, e);
    return 0;
}
