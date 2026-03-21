#include <stddef.h>
#include <stdio.h>

int main(void) {
    int a[6] = {3, 5, 7, 11, 13, 17};
    int* p = &a[1];
    int* q = &a[4];

    ptrdiff_t d1 = q - p;
    ptrdiff_t d2 = &a[5] - p;
    int c1 = (p < q);
    int c2 = (q > a);
    int sample = p[2] + q[-1];

    printf("%td %td %d %d %d\n", d1, d2, c1, c2, sample);
    return 0;
}
