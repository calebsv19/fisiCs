#include <stdio.h>

typedef struct Axis5W6Ring {
    unsigned int values[8];
    unsigned int head;
} Axis5W6Ring;

static unsigned int axis5_w6_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w6_init(Axis5W6Ring* r) {
    for (int i = 0; i < 8; ++i) r->values[i] = 0u;
    r->head = 0u;
}

static void axis5_w6_step(Axis5W6Ring* r, unsigned int lane, unsigned int value) {
    r->head = (r->head + (lane & 3u)) & 7u;
    unsigned int idx = (r->head + (lane & 7u)) & 7u;
    r->values[idx] += (value & 0xffu) ^ ((lane & 7u) << 3);
}

static unsigned int axis5_w6_canonical_sig(const Axis5W6Ring* r) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < 8; ++i) {
        unsigned int idx = (r->head + (unsigned int)i) & 7u;
        h = axis5_w6_mix(h, r->values[idx]);
    }
    return h;
}

static void axis5_w6_rotate_storage(Axis5W6Ring* r, unsigned int rot) {
    unsigned int tmp[8];
    for (int i = 0; i < 8; ++i) tmp[(i + (int)rot) & 7] = r->values[i];
    for (int i = 0; i < 8; ++i) r->values[i] = tmp[i];
    r->head = (r->head + rot) & 7u;
}

int main(void) {
    const unsigned int lanes[] = {1u, 3u, 2u, 0u, 7u, 4u, 2u, 6u, 1u, 5u};
    const unsigned int vals[] = {9u, 4u, 7u, 3u, 8u, 6u, 5u, 2u, 11u, 10u};

    Axis5W6Ring base;
    Axis5W6Ring rotated;
    axis5_w6_init(&base);
    axis5_w6_init(&rotated);

    for (int i = 0; i < 10; ++i) axis5_w6_step(&base, lanes[i], vals[i]);
    rotated = base;
    axis5_w6_rotate_storage(&rotated, 3u);

    const unsigned int sig_base = axis5_w6_canonical_sig(&base);
    const unsigned int sig_rot = axis5_w6_canonical_sig(&rotated);
    const unsigned int same = (sig_base == sig_rot) ? 1u : 0u;

    printf("%u %u %u\n", sig_base, sig_rot, same);
    return 0;
}
