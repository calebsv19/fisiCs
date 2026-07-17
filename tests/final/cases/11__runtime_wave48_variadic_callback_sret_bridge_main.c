#include <stdio.h>

struct pair48 {
    int a;
    int b;
};

struct big48 {
    int lanes[5];
    long total;
};

typedef int (*pair48_cb)(struct pair48 item, int salt);

struct big48 wave48_variadic_callback_sret_bridge(int seed, pair48_cb cb, int count, ...);

static int wave48_pair_fold(struct pair48 item, int salt) {
    return item.a * salt + item.b;
}

int main(void) {
    struct big48 got = wave48_variadic_callback_sret_bridge(7, wave48_pair_fold, 4, 3, 5, 8, 2);
    printf("%d %d %d %d %ld\n", got.lanes[0], got.lanes[1], got.lanes[2], got.lanes[3], got.total);
    return 0;
}
