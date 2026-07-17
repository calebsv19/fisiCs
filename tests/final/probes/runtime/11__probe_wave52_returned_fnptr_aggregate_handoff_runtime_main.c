#include <stdio.h>

struct wave52_route_pair {
    int left;
    int right;
};

struct wave52_route_packet {
    struct wave52_route_pair pair;
    int route_bias;
};

struct wave52_route_result {
    long total;
    int route;
    int echo;
};

typedef struct wave52_route_pair (*wave52_route_mix_fn)(struct wave52_route_pair pair, int salt);
typedef struct wave52_route_result (*wave52_route_fn)(struct wave52_route_packet packet,
                                                      wave52_route_mix_fn mix,
                                                      int salt);

wave52_route_fn wave52_returned_fnptr_aggregate_handoff(int selector,
                                                        struct wave52_route_packet seed);

static struct wave52_route_pair wave52_mix(struct wave52_route_pair pair, int salt) {
    struct wave52_route_pair out;
    out.left = pair.right + salt * 2 + pair.left;
    out.right = pair.left * 4 - pair.right + salt;
    return out;
}

int main(void) {
    struct wave52_route_packet seed = {{7, 4}, 5};
    wave52_route_fn fn = wave52_returned_fnptr_aggregate_handoff(6, seed);
    struct wave52_route_result got = fn(seed, wave52_mix, 3);
    printf("%ld %d %d\n", got.total, got.route, got.echo);
    return 0;
}
