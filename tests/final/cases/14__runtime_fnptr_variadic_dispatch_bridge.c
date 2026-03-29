#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

typedef int (*variadic_lane_fn)(int count, ...);

static int add_lane(int count, ...) {
    va_list ap;
    va_start(ap, count);

    int acc = 0;
    for (int i = 0; i < count; ++i) {
        acc += va_arg(ap, int);
    }

    va_end(ap);
    return acc;
}

static int zigzag_lane(int count, ...) {
    va_list ap;
    va_start(ap, count);

    int acc = 0;
    for (int i = 0; i < count; ++i) {
        int v = va_arg(ap, int);
        acc += (i & 1) ? v : -v;
    }

    va_end(ap);
    return acc;
}

static variadic_lane_fn choose_lane(int which) {
    return which ? add_lane : zigzag_lane;
}

int main(void) {
    variadic_lane_fn f0 = choose_lane(1);
    variadic_lane_fn f1 = choose_lane(0);
    variadic_lane_fn table[2] = {f0, f1};

    int a = f0(4, 1, 2, 3, 4);
    int b = f1(3, 10, 20, 5);
    int c = table[1](4, 8, 6, 4, 2);

    printf("%d %d %d\n", a, b, c);
    return 0;
}
