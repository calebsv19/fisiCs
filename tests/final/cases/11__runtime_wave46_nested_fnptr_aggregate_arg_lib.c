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

Result46A wave46_nested_fnptr_aggregate_arg_dispatch(Chooser46A chooser, Payload46A payload, int tag) {
    Leaf46A leaf = chooser(tag);
    Payload46A next = {payload.base + 1, payload.delta + 2, payload.scale};
    int first = leaf(payload, tag);
    int second = leaf(next, tag + 1);
    Result46A result = {first + second, first - second, tag + payload.scale};
    return result;
}
