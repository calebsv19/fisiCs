#include <stdio.h>

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
                                                                          int bias);

static struct wave50_nested_payload wave50_nested_remap(struct wave50_nested_payload item, int step) {
    struct wave50_nested_payload out;
    out.id = item.id + step + item.u.pair.lo;
    out.u.pair.lo = item.u.pair.hi + step;
    out.u.pair.hi = item.u.pair.lo * 2 + item.id;
    return out;
}

int main(void) {
    struct wave50_nested_payload seed;
    struct wave50_nested_envelope got;
    seed.id = 3;
    seed.u.pair.lo = 4;
    seed.u.pair.hi = 9;
    got = wave50_nested_callback_payload_preservation(seed, wave50_nested_remap, 5);
    printf("%d %d %d %d %ld\n",
           got.slots[0].id,
           got.slots[0].u.pair.lo,
           got.slots[1].id,
           got.slots[1].u.pair.hi,
           got.stamp);
    return 0;
}
