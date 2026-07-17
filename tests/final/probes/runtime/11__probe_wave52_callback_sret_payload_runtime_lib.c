struct wave52_sret_cell {
    int lane[4];
};

struct wave52_sret_payload {
    struct wave52_sret_cell first;
    struct wave52_sret_cell last;
    long sum;
    int tag;
};

typedef struct wave52_sret_payload (*wave52_sret_cb)(struct wave52_sret_cell cell, int salt);

struct wave52_sret_payload wave52_callback_sret_payload(struct wave52_sret_cell seed,
                                                        wave52_sret_cb cb,
                                                        int rounds,
                                                        int salt) {
    struct wave52_sret_payload out;
    struct wave52_sret_cell cur = seed;
    int r;

    out.first = seed;
    out.last = seed;
    out.sum = salt + rounds;
    out.tag = rounds;

    for (r = 0; r < rounds; r++) {
        struct wave52_sret_payload next = cb(cur, salt + r);
        cur = next.last;
        out.first.lane[r & 3] += (int)(next.sum % 19);
        out.last = cur;
        out.sum += next.sum + cur.lane[(r + 1) & 3];
        out.tag += next.tag + cur.lane[r & 3];
    }

    return out;
}
