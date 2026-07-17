struct wave50_ring_vec {
    int lane[4];
};

struct wave50_ring_packet {
    struct wave50_ring_vec left;
    struct wave50_ring_vec right;
    long checksum;
};

typedef struct wave50_ring_vec (*wave50_ring_cb)(struct wave50_ring_vec item, int bias);

struct wave50_ring_packet wave50_byval_sret_callback_payload_ring(struct wave50_ring_vec seed,
                                                                  wave50_ring_cb cb,
                                                                  int rounds) {
    struct wave50_ring_packet out;
    struct wave50_ring_vec cur = seed;
    int r;

    out.left = seed;
    out.right = seed;
    out.checksum = rounds;

    for (r = 0; r < rounds; r++) {
        cur = cb(cur, r + 2);
        out.right = cur;
        out.left.lane[r % 4] += cur.lane[(r + 1) % 4] + r;
        out.checksum += cur.lane[r % 4] * (r + 3) + out.left.lane[r % 4];
    }

    return out;
}
