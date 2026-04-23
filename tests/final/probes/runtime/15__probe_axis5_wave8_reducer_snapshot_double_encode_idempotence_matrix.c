#include <stdio.h>

typedef struct Axis5W8State {
    unsigned int bins[6];
    unsigned int salt;
} Axis5W8State;

static unsigned int axis5_w8_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w8_init(Axis5W8State* s, unsigned int salt) {
    for (int i = 0; i < 6; ++i) s->bins[i] = 0u;
    s->salt = salt;
}

static void axis5_w8_apply(Axis5W8State* s, unsigned int lane, unsigned int value) {
    unsigned int idx = lane % 6u;
    s->bins[idx] += (value & 0xffu) ^ ((lane & 7u) << 3);
}

static void axis5_w8_encode(const Axis5W8State* s, unsigned int wire[7]) {
    for (int i = 0; i < 6; ++i) wire[i] = s->bins[i] ^ (0xA660u + (unsigned int)i);
    wire[6] = s->salt ^ 0x5A5Au;
}

static void axis5_w8_decode(Axis5W8State* s, const unsigned int wire[7]) {
    for (int i = 0; i < 6; ++i) s->bins[i] = wire[i] ^ (0xA660u + (unsigned int)i);
    s->salt = wire[6] ^ 0x5A5Au;
}

static unsigned int axis5_w8_sig(const Axis5W8State* s) {
    unsigned int h = 0x811c9dc5u ^ s->salt;
    for (int i = 0; i < 6; ++i) h = axis5_w8_mix(h, s->bins[i]);
    return h;
}

int main(void) {
    const unsigned int lanes[] = {0u, 3u, 1u, 5u, 2u, 4u, 0u, 1u, 3u, 5u};
    const unsigned int vals[] = {8u, 2u, 6u, 9u, 4u, 3u, 11u, 7u, 5u, 10u};
    unsigned int wire1[7];
    unsigned int wire2[7];

    Axis5W8State base;
    Axis5W8State roundtrip;

    axis5_w8_init(&base, 0x3141u);
    for (int i = 0; i < 10; ++i) axis5_w8_apply(&base, lanes[i], vals[i]);

    axis5_w8_encode(&base, wire1);
    axis5_w8_decode(&roundtrip, wire1);
    axis5_w8_encode(&roundtrip, wire2);
    axis5_w8_decode(&roundtrip, wire2);

    const unsigned int sig_base = axis5_w8_sig(&base);
    const unsigned int sig_rt = axis5_w8_sig(&roundtrip);
    const unsigned int same = (sig_base == sig_rt) ? 1u : 0u;

    printf("%u %u %u\n", sig_base, sig_rt, same);
    return 0;
}
