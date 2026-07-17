#include <stdio.h>

struct cell48 {
    int id;
    int v[3];
};

struct pack48 {
    struct cell48 left;
    struct cell48 right;
    int bias;
};

struct ret48 {
    long sum;
    int route;
    int echo;
};

typedef struct ret48 (*route48_fn)(struct pack48 pack, int salt);

route48_fn wave48_pick_route(int selector, struct pack48 seed);

int main(void) {
    struct pack48 pack = {
        {2, {4, 6, 8}},
        {5, {1, 3, 7}},
        3,
    };
    route48_fn fn = wave48_pick_route(5, pack);
    struct ret48 got = fn(pack, 4);
    printf("%ld %d %d\n", got.sum, got.route, got.echo);
    return 0;
}
