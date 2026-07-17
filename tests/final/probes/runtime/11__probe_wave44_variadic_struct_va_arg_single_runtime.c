#include <stdarg.h>
#include <stdio.h>

struct Wave44Single {
    int a;
    int b;
};

static int wave44_struct_va_arg_single(int seed, ...) {
    va_list args;
    struct Wave44Single item;

    va_start(args, seed);
    item = va_arg(args, struct Wave44Single);
    va_end(args);

    return seed + item.a * 3 + item.b * 5;
}

int main(void) {
    struct Wave44Single item = { 7, 31 };
    printf("%d\n", wave44_struct_va_arg_single(5, item));
    return 0;
}
