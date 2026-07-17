#include <stdio.h>

enum Route {
    ROUTE_A = 6,
    ROUTE_B = 17
};

struct Pair {
    unsigned char left;
    signed char right;
};

struct Packet {
    enum Route route;
    struct Pair pairs[2];
    unsigned short seed;
};

typedef int (*packet_fn)(struct Packet *, int);

static int fold_left(struct Packet *packet, int index) {
    struct Pair *pair = &packet->pairs[index];
    unsigned int promoted = (unsigned int)(unsigned char)(pair->left + (unsigned char)packet->route);
    return (int)(promoted & 255u) + (int)pair->right + (int)(packet->seed & 15u);
}

static int fold_right(struct Packet *packet, int index) {
    struct Pair *pair = &packet->pairs[index];
    int signed_lane = (int)(signed char)(pair->right - (signed char)packet->route);
    return signed_lane + (int)(unsigned char)(pair->left + (unsigned char)packet->seed);
}

static int dispatch(packet_fn fn, struct Packet *packet, int index) {
    packet_fn selected = packet->route == ROUTE_B ? fold_right : fn;
    return selected(packet, index);
}

int main(void) {
    struct Packet packets[2] = {
        {ROUTE_A, {{240u, -5}, {31u, 9}}, 21u},
        {ROUTE_B, {{88u, 12}, {201u, -18}}, 34u}
    };
    packet_fn primary = fold_left;
    struct Packet *chosen = 1 ? &packets[0] : &packets[1];

    int first = dispatch(primary, chosen, 0);
    int second = dispatch(primary, &packets[0], 1);
    int third = dispatch(primary, &packets[1], 1);
    printf("%d %d %d\n", first, second, third);
    return 0;
}
