#include <stdarg.h>

struct Wave45Node {
    int id;
    int values[3];
};

int wave45_variadic_nested_pair(int seed, int rounds, ...) {
    va_list args;
    struct Wave45Node left;
    struct Wave45Node right;
    int scale;
    int acc;
    int i;

    va_start(args, rounds);
    left = va_arg(args, struct Wave45Node);
    scale = va_arg(args, int);
    right = va_arg(args, struct Wave45Node);
    va_end(args);

    acc = seed + scale * rounds;
    for (i = 0; i < 3; ++i) {
        acc += left.values[i] * (i + 1);
        acc += right.values[i] * (i + 3);
    }

    return acc + left.id * 5 + right.id * 7;
}
