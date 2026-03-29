#include <stddef.h>
#include <stdio.h>

int main(void) {
    int a[6] = {4, 9, 16, 25, 36, 49};
    int* begin = &a[0];
    int* last = &a[5];
    int* end = &a[6];

    ptrdiff_t span = end - begin;
    ptrdiff_t tail = end - last;
    int cmp1 = (end > last);
    int cmp2 = (begin < end);
    int edge = *(end - 1);

    printf("%td %td %d %d %d\n", span, tail, cmp1, cmp2, edge);
    return 0;
}
