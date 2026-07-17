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

static struct ret48 wave48_route_add(struct pack48 pack, int salt) {
    struct ret48 out;
    out.sum = pack.left.id + pack.right.id + pack.bias + salt;
    out.sum += pack.left.v[0] + pack.left.v[1] + pack.left.v[2];
    out.sum += pack.right.v[0] + pack.right.v[1] + pack.right.v[2];
    out.route = 1;
    out.echo = pack.left.v[0] * pack.right.v[2];
    return out;
}

static struct ret48 wave48_route_mix(struct pack48 pack, int salt) {
    struct ret48 out;
    out.sum = pack.left.id * 3 - pack.right.id + pack.bias * salt;
    out.sum += pack.left.v[2] * pack.right.v[0];
    out.route = 2;
    out.echo = pack.right.v[1] - pack.left.v[1] + salt;
    return out;
}

route48_fn wave48_pick_route(int selector, struct pack48 seed) {
    if (((selector + seed.bias) & 1) == 0) {
        return wave48_route_add;
    }
    return wave48_route_mix;
}
