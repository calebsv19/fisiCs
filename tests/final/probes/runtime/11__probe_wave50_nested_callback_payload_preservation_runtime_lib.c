struct wave50_nested_payload {
    int id;
    union {
        struct {
            int lo;
            int hi;
        } pair;
        int raw[2];
    } u;
};

struct wave50_nested_envelope {
    struct wave50_nested_payload slots[2];
    long stamp;
};

typedef struct wave50_nested_payload (*wave50_nested_cb)(struct wave50_nested_payload item, int step);

struct wave50_nested_envelope wave50_nested_callback_payload_preservation(struct wave50_nested_payload seed,
                                                                          wave50_nested_cb cb,
                                                                          int bias) {
    struct wave50_nested_envelope out;
    out.slots[0] = cb(seed, bias);
    out.slots[1] = cb(out.slots[0], bias + 3);
    out.stamp = out.slots[0].id * 17L
        + out.slots[0].u.pair.lo * 3L
        + out.slots[0].u.pair.hi * 5L
        + out.slots[1].id * 7L
        + out.slots[1].u.raw[0] * 11L
        + out.slots[1].u.raw[1] * 13L;
    return out;
}
