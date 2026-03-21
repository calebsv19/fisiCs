#include <stddef.h>
#include <stdio.h>

static int load_int_index(const int* base, int idx) {
    return *(base + idx);
}

static int load_size_index(const int* base, size_t idx) {
    return *(base + idx);
}

static ptrdiff_t diff_from(const int* lhs, const int* rhs) {
    return lhs - rhs;
}

int main(void) {
    int a[8] = {3, 7, 11, 15, 19, 23, 27, 31};
    const int* mid = &a[3];
    int left = load_int_index(mid, -2);
    int right = load_int_index(mid, 3);
    size_t sidx = 6u;
    int abs_val = load_size_index(a, sidx);
    ptrdiff_t d1 = diff_from(&a[7], &a[0]);
    ptrdiff_t d2 = diff_from(mid, &a[0]);

    printf("%d %d %d %td %td\n", left, right, abs_val, d1, d2);
    return 0;
}
