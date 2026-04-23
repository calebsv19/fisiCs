#include <stdio.h>

typedef struct Axis5W2Event {
    unsigned int op;
    unsigned int key;
    unsigned int amount;
} Axis5W2Event;

typedef struct Axis5W2State {
    unsigned int bins[8];
} Axis5W2State;

static void axis5_w2_reset(Axis5W2State* s) {
    for (int i = 0; i < 8; ++i) s->bins[i] = 0u;
}

static void axis5_w2_apply(Axis5W2State* s, Axis5W2Event e) {
    unsigned int idx = e.key & 7u;
    unsigned int cur = s->bins[idx];
    if ((e.op & 1u) == 0u) {
        s->bins[idx] = cur + (e.amount & 31u);
    } else {
        unsigned int dec = e.amount & 31u;
        s->bins[idx] = (cur > dec) ? (cur - dec) : 0u;
    }
}

static void axis5_w2_encode(const Axis5W2State* s, unsigned int wire[8]) {
    for (int i = 0; i < 8; ++i) wire[i] = (s->bins[i] ^ (0xA5u + (unsigned int)i));
}

static void axis5_w2_decode(Axis5W2State* s, const unsigned int wire[8]) {
    for (int i = 0; i < 8; ++i) s->bins[i] = (wire[i] ^ (0xA5u + (unsigned int)i));
}

static unsigned int axis5_w2_sig(const Axis5W2State* s) {
    unsigned int h = 0x811c9dc5u;
    for (int i = 0; i < 8; ++i) {
        h ^= s->bins[i] + 0x9e3779b9u + (h << 6) + (h >> 2);
    }
    return h;
}

int main(void) {
    const Axis5W2Event stream[] = {
        {0u, 1u, 5u}, {0u, 3u, 9u}, {1u, 1u, 2u}, {0u, 7u, 4u},
        {0u, 3u, 6u}, {1u, 3u, 8u}, {0u, 0u, 3u}, {1u, 7u, 1u},
        {0u, 2u, 7u}, {0u, 4u, 2u}, {1u, 2u, 5u}, {0u, 6u, 8u},
    };
    Axis5W2State full;
    Axis5W2State windowed;
    unsigned int wire[8];

    axis5_w2_reset(&full);
    axis5_w2_reset(&windowed);

    for (int i = 0; i < 12; ++i) axis5_w2_apply(&full, stream[i]);

    for (int base = 0; base < 12; base += 4) {
        for (int i = base; i < base + 4; ++i) axis5_w2_apply(&windowed, stream[i]);
        axis5_w2_encode(&windowed, wire);
        axis5_w2_reset(&windowed);
        axis5_w2_decode(&windowed, wire);
    }

    const unsigned int sig_full = axis5_w2_sig(&full);
    const unsigned int sig_window = axis5_w2_sig(&windowed);
    const unsigned int stable = (sig_full == sig_window) ? 1u : 0u;

    printf("%u %u %u\n", sig_full, sig_window, stable);
    return 0;
}
