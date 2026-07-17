#include <stdio.h>

typedef struct Pair47B {
    int x;
    int y;
} Pair47B;

typedef struct Frame47B {
    Pair47B pairs[2];
    int tag;
} Frame47B;

typedef int (*Reducer47B)(Frame47B, int);

int wave47_nested_aggregate_argument_callback(Reducer47B reducer, Frame47B first, Frame47B second, int route);

static int reduce_frame47(Frame47B frame, int route) {
    return frame.pairs[0].x + frame.pairs[0].y +
        frame.pairs[1].x + frame.pairs[1].y + frame.tag + route;
}

int main(void) {
    Frame47B left = {{{1, 2}, {3, 4}}, 5};
    Frame47B right = {{{6, 7}, {8, 9}}, 10};
    int total = wave47_nested_aggregate_argument_callback(reduce_frame47, left, right, 4);
    if (total != 105) return 1;
    printf("%d %d\n", total, total - left.tag - right.tag);
    return 0;
}
