#include <limits.h>
#include <stdio.h>

int main(void) {
    int si = -1;
    unsigned int ui = 1u;
    unsigned int umax = UINT_MAX;

    int a = (si < ui);
    int b = (si > ui);
    int c = (si == umax);
    int d = ((unsigned int)si == umax);
    int e = ((long long)-2 < (unsigned long long)1);

    printf("%d %d %d %d %d\n", a, b, c, d, e);
    return 0;
}
