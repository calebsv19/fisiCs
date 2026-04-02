#include <limits.h>
#include <stdio.h>

int main(void) {
    long sl = -1;
    unsigned long ul = 1ul;
    unsigned long ulmax = ULONG_MAX;
    long long sll = -1;
    unsigned long long ull = 1ull;

    int a = (sl < ul);
    int b = (sl > ul);
    int c = ((unsigned long)sl == ulmax);
    int d = (sll < ull);
    int e = (sll > ull);

    printf("%d %d %d %d %d\n", a, b, c, d, e);
    return 0;
}
