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

static struct wave51_cb_result wave51_route_add(struct wave51_cb_packet packet,
                                                struct wave51_cb_pair (*mix)(struct wave51_cb_pair item, int salt),
                                                int salt) {
    struct wave51_cb_pair m = mix(packet.pair, salt);
    struct wave51_cb_result out;
    out.total = (long)m.left * 5 + (long)m.right * 7 + packet.bias;
    out.route = 1;
    out.echo = m.left - m.right + salt;
    return out;
}

static struct wave51_cb_result wave51_route_fold(struct wave51_cb_packet packet,
                                                 struct wave51_cb_pair (*mix)(struct wave51_cb_pair item, int salt),
                                                 int salt) {
    struct wave51_cb_pair m = mix(packet.pair, salt + packet.bias);
    struct wave51_cb_result out;
    out.total = (long)m.left * 11 - (long)m.right * 2 + packet.bias * salt;
    out.route = 2;
    out.echo = m.left + m.right + packet.bias;
    return out;
}

wave51_cb_route_fn wave51_returned_fnptr_aggregate_callback(int selector,
                                                            struct wave51_cb_packet seed) {
    if (((selector + seed.bias) & 3) == 0) {
        return wave51_route_fold;
    }
    return wave51_route_add;
}
