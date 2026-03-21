#include <stddef.h>
#include <stdio.h>

struct Pair {
    int a;
    int b;
};

int main(void) {
    struct Pair items[4] = {{1, 2}, {3, 4}, {5, 6}, {7, 8}};
    struct Pair* base = &items[0];
    struct Pair* tail = &items[3];
    ptrdiff_t gap = tail - base;
    int i = 0;

    for (i = 0; i < 4; ++i) {
        (base + i)->a += i;
        (base + i)->b -= i;
    }

    printf(
        "%td %d %d %d %d\n",
        gap,
        items[0].a + items[0].b,
        items[1].a + items[1].b,
        items[2].a + items[2].b,
        items[3].a + items[3].b
    );
    return 0;
}
