#include <stdio.h>

struct wave51_cb_pair {
    int left;
    int right;
};

struct wave51_cb_packet {
    struct wave51_cb_pair pair;
    int bias;
};

struct wave51_cb_result {
    long total;
    int route;
    int echo;
};

typedef struct wave51_cb_result (*wave51_cb_route_fn)(struct wave51_cb_packet packet,
                                                      struct wave51_cb_pair (*mix)(struct wave51_cb_pair item, int salt),
                                                      int salt);

wave51_cb_route_fn wave51_returned_fnptr_aggregate_callback(int selector,
                                                            struct wave51_cb_packet seed);

static struct wave51_cb_pair wave51_mix_pair(struct wave51_cb_pair item, int salt) {
    struct wave51_cb_pair out;
    out.left = item.left + salt * 2;
    out.right = item.right * 3 - salt;
    return out;
}

int main(void) {
    struct wave51_cb_packet seed = {{6, 5}, 4};
    wave51_cb_route_fn fn = wave51_returned_fnptr_aggregate_callback(8, seed);
    struct wave51_cb_result got = fn(seed, wave51_mix_pair, 3);
    printf("%ld %d %d\n", got.total, got.route, got.echo);
    return 0;
}
