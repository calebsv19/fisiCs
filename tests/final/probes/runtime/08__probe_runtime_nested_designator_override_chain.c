#include <stdio.h>

struct Node {
    int a;
    int b;
};

struct State {
    struct Node row[3];
    int total;
};

static struct State build_state(void) {
    struct State state = {
        .row[1].b = 5,
        .row[0].a = 1,
        .row[1].a = 4,
        .row[1].b = 7,
        .row[2] = {9, 10},
        .total = 31,
    };
    return state;
}

int main(void) {
    struct State state = build_state();
    printf("%d %d %d %d %d %d %d\n",
           state.row[0].a,
           state.row[0].b,
           state.row[1].a,
           state.row[1].b,
           state.row[2].a,
           state.row[2].b,
           state.total);
    return 0;
}
