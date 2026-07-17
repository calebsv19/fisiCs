#include <stdio.h>

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
                                                                  int rounds);

static struct wave50_ring_vec wave50_ring_mix(struct wave50_ring_vec item, int bias) {
    struct wave50_ring_vec out;
    int i;
    for (i = 0; i < 4; i++) {
        out.lane[i] = item.lane[(i + 1) % 4] + bias * (i + 1) + item.lane[i];
    }
    return out;
}

int main(void) {
    struct wave50_ring_vec seed = {{2, 4, 6, 8}};
    struct wave50_ring_packet got = wave50_byval_sret_callback_payload_ring(seed, wave50_ring_mix, 3);
    printf("%d %d %d %d %ld\n", got.left.lane[0], got.left.lane[2], got.right.lane[1], got.right.lane[3], got.checksum);
    return 0;
}
