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

static struct wave52_route_result wave52_route_left(struct wave52_route_packet packet,
                                                    wave52_route_mix_fn mix,
                                                    int salt) {
    struct wave52_route_pair m = mix(packet.pair, salt + packet.route_bias);
    struct wave52_route_result out;
    out.total = (long)m.left * 13 + (long)m.right * 3 + packet.route_bias;
    out.route = 11;
    out.echo = m.left - m.right + salt;
    return out;
}

static struct wave52_route_result wave52_route_right(struct wave52_route_packet packet,
                                                     wave52_route_mix_fn mix,
                                                     int salt) {
    struct wave52_route_pair m = mix(packet.pair, salt);
    struct wave52_route_result out;
    out.total = (long)m.left * 5 - (long)m.right * 7 + packet.route_bias * salt;
    out.route = 17;
    out.echo = m.left + m.right + packet.route_bias;
    return out;
}

wave52_route_fn wave52_returned_fnptr_aggregate_handoff(int selector,
                                                        struct wave52_route_packet seed) {
    if (((selector + seed.route_bias) & 1) != 0) {
        return wave52_route_left;
    }
    return wave52_route_right;
}
