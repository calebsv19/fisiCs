#include <stdarg.h>

struct Wave44Mini {
    int a;
    int b;
};

int wave44_struct_va_arg_min(int seed, ...) {
    va_list args;
    struct Wave44Mini item;

    va_start(args, seed);
    item = va_arg(args, struct Wave44Mini);
    va_end(args);

    return seed + item.a * 3 + item.b * 5;
}
