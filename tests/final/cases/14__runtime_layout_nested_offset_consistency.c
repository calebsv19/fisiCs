#include <stddef.h>
#include <stdio.h>

struct Inner {
    short s;
    char t;
};

struct Outer {
    char head;
    struct Inner in[2];
    int tail;
};

int main(void) {
    struct Outer arr[4];
    for (int i = 0; i < 4; ++i) {
        arr[i].head = (char)(i + 1);
        arr[i].in[0].s = (short)(i * 10 + 1);
        arr[i].in[0].t = (char)(i * 2 + 3);
        arr[i].in[1].s = (short)(i * 10 + 2);
        arr[i].in[1].t = (char)(i * 2 + 4);
        arr[i].tail = i * 100 + 7;
    }

    char* base = (char*)&arr[0];
    ptrdiff_t actual = (char*)&arr[3].in[1].t - base;
    ptrdiff_t expect = (ptrdiff_t)(3 * sizeof(struct Outer)) +
                       (ptrdiff_t)offsetof(struct Outer, in) +
                       (ptrdiff_t)sizeof(struct Inner) +
                       (ptrdiff_t)offsetof(struct Inner, t);
    int k1 = (actual == expect);

    ptrdiff_t tail_actual = (char*)&arr[2].tail - (char*)&arr[1].tail;
    int k2 = (tail_actual == (ptrdiff_t)sizeof(struct Outer));

    int sum = 0;
    for (int i = 0; i < 4; ++i) {
        sum += arr[i].in[0].s + arr[i].in[1].s + arr[i].tail;
    }

    printf("%d %d %d\n", k1, k2, sum);
    return 0;
}
