#include <stdio.h>

typedef int (*binop_t)(int, int);
typedef binop_t (*resolver_t)(int);
typedef resolver_t (*router_t)(int);

static int add_op(int a, int b) { return a + b; }
static int sub_op(int a, int b) { return a - b; }
static int xor_op(int a, int b) { return (a ^ b) & 255; }

static binop_t resolve_mode(int mode) {
    switch (mode % 3) {
        case 0: return add_op;
        case 1: return sub_op;
        default: return xor_op;
    }
}

static resolver_t route_level1(int seed) {
    (void)seed;
    return resolve_mode;
}

static resolver_t route_level2(int seed) {
    (void)seed;
    return resolve_mode;
}

static router_t pick_router(int idx) {
    router_t table[2] = {route_level1, route_level2};
    return table[idx & 1];
}

int main(void) {
    router_t routes[2] = {route_level1, route_level2};
    int acc = 9;

    for (int i = 0; i < 12; ++i) {
        router_t router = (i & 2) ? pick_router(i) : routes[i & 1];
        resolver_t resolver = router(i + 3);
        binop_t op = resolver(i + acc + 1);
        acc += op(i * 3 + 1, i + 7);
        acc ^= (i << 1);
        acc &= 0x7fffffff;
    }

    printf("%d\n", acc);
    return 0;
}
