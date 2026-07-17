#include <stdarg.h>

struct Wave45Box {
    short tag;
    short pad;
    union {
        int lanes[3];
        struct {
            int a;
            int b;
            int c;
        } triple;
    } u;
};

typedef int (*Wave45Mixer)(struct Wave45Box, int);

int wave45_variadic_union_fnptr(int seed, ...) {
    va_list args;
    struct Wave45Box first;
    struct Wave45Box second;
    int bias;
    Wave45Mixer mixer;

    va_start(args, seed);
    first = va_arg(args, struct Wave45Box);
    bias = va_arg(args, int);
    mixer = va_arg(args, Wave45Mixer);
    second = va_arg(args, struct Wave45Box);
    va_end(args);

    return seed + bias +
           mixer(first, second.u.lanes[2]) +
           first.u.triple.b * 3 +
           second.u.triple.a * 5 +
           first.tag * 7 +
           second.tag * 11;
}
