#include <stdio.h>

typedef struct Payload46A {
    int base;
    int delta;
    int scale;
} Payload46A;

typedef struct Result46A {
    int total;
    int spread;
    int route;
} Result46A;

typedef int (*Leaf46A)(Payload46A, int);
typedef Leaf46A (*Chooser46A)(int);

Result46A wave46_nested_fnptr_aggregate_arg_dispatch(Chooser46A chooser, Payload46A payload, int tag);

static int leaf_even(Payload46A payload, int tag) {
    return payload.base * payload.scale + payload.delta + tag;
}

static int leaf_odd(Payload46A payload, int tag) {
    return payload.base + payload.delta * payload.scale - tag;
}

static Leaf46A choose_leaf(int tag) {
    return (tag & 1) ? leaf_odd : leaf_even;
}

int main(void) {
    Payload46A payload = {3, 7, 5};
    Result46A result = wave46_nested_fnptr_aggregate_arg_dispatch(choose_leaf, payload, 4);
    if (result.total != 60) return 1;
    if (result.spread != -8) return 2;
    if (result.route != 9) return 3;
    printf("%d %d %d\n", result.total, result.spread, result.route);
    return 0;
}
