struct wave51_chain_cell {
    int lane[4];
};

struct wave51_chain_payload {
    struct wave51_chain_cell head;
    struct wave51_chain_cell tail;
    long checksum;
    int rounds;
};

typedef struct wave51_chain_cell (*wave51_chain_cb)(struct wave51_chain_cell item, int salt);

struct wave51_chain_payload wave51_sret_byval_callback_chain(struct wave51_chain_cell seed,
                                                             wave51_chain_cb cb,
                                                             int rounds,
                                                             int salt) {
    struct wave51_chain_payload out;
    struct wave51_chain_cell cur = seed;
    int r;

    out.head = seed;
    out.tail = seed;
    out.checksum = salt + rounds;
    out.rounds = rounds;

    for (r = 0; r < rounds; r++) {
        cur = cb(cur, salt + r);
        out.tail = cur;
        out.head.lane[(r + 1) & 3] += cur.lane[r & 3] + salt;
        out.checksum += (long)cur.lane[(r + 2) & 3] * (r + 3) + out.head.lane[(r + 1) & 3];
    }

    return out;
}
