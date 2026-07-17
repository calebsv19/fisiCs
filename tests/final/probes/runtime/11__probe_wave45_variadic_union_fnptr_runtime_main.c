#include <stdio.h>

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

int wave45_variadic_union_fnptr(int seed, ...);

static int local_mix(struct Wave45Box box, int salt) {
    return box.u.lanes[0] * 13 + box.u.lanes[2] * 2 + salt;
}

int main(void) {
    struct Wave45Box first = { 3, 0, { { 4, 6, 8 } } };
    struct Wave45Box second = { 5, 0, { { 10, 12, 14 } } };

    printf("%d\n", wave45_variadic_union_fnptr(29, first, 17, local_mix, second));
    return 0;
}
