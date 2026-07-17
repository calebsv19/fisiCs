#include <stdio.h>

struct Wave44Pair {
    int x;
    int y;
};

typedef int (*Wave44Callback)(struct Wave44Pair, int);

int wave44_variadic_bridge(int seed, int count, ...);

static int local_callback(struct Wave44Pair pair, int salt) {
    return pair.x * 7 + pair.y * 3 + (salt % 29);
}

int main(void) {
    struct Wave44Pair left = { 5, 17 };
    struct Wave44Pair right = { 23, 41 };
    int folded = wave44_variadic_bridge(
        13,
        4,
        left,
        2.5,
        10000000037LL,
        local_callback,
        right);

    printf("%d\n", folded);
    return 0;
}
