typedef struct Pair47B {
    int x;
    int y;
} Pair47B;

typedef struct Frame47B {
    Pair47B pairs[2];
    int tag;
} Frame47B;

typedef int (*Reducer47B)(Frame47B, int);

int wave47_nested_aggregate_argument_callback(Reducer47B reducer, Frame47B first, Frame47B second, int route) {
    Frame47B mixed = first;
    mixed.pairs[1] = second.pairs[0];
    mixed.tag = first.tag + second.tag + route;
    return reducer(first, route) + reducer(second, route + 1) + reducer(mixed, route + 2);
}
