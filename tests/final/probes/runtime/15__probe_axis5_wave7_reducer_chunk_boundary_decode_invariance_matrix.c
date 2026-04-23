#include <stdio.h>

typedef struct Axis5W7Run {
    unsigned int key;
    unsigned int len;
    int delta;
} Axis5W7Run;

typedef struct Axis5W7State {
    int bins[8];
} Axis5W7State;

static unsigned int axis5_w7_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w7_reset(Axis5W7State* s) {
    for (int i = 0; i < 8; ++i) s->bins[i] = 0;
}

static void axis5_w7_apply_step(Axis5W7State* s, unsigned int key, int delta) {
    s->bins[key & 7u] += delta;
}

static void axis5_w7_apply_runs(Axis5W7State* s, const Axis5W7Run* runs, int n) {
    for (int i = 0; i < n; ++i) {
        for (unsigned int k = 0; k < runs[i].len; ++k) {
            axis5_w7_apply_step(s, runs[i].key, runs[i].delta);
        }
    }
}

static void axis5_w7_apply_runs_chunked(
    Axis5W7State* s,
    const Axis5W7Run* runs,
    int n,
    unsigned int chunk
) {
    for (int i = 0; i < n; ++i) {
        unsigned int left = runs[i].len;
        while (left > 0u) {
            unsigned int take = (left > chunk) ? chunk : left;
            for (unsigned int k = 0; k < take; ++k) {
                axis5_w7_apply_step(s, runs[i].key, runs[i].delta);
            }
            left -= take;
        }
    }
}

static unsigned int axis5_w7_sig(const Axis5W7State* s) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < 8; ++i) {
        h = axis5_w7_mix(h, (unsigned int)i);
        h = axis5_w7_mix(h, (unsigned int)(s->bins[i] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W7Run runs[] = {
        {0u, 4u, +3}, {3u, 3u, -2}, {1u, 5u, +1}, {0u, 2u, -3},
        {6u, 7u, +2}, {3u, 1u, +5}, {1u, 4u, -1}, {6u, 2u, -2},
    };
    Axis5W7State direct;
    Axis5W7State chunked;

    axis5_w7_reset(&direct);
    axis5_w7_reset(&chunked);
    axis5_w7_apply_runs(&direct, runs, 8);
    axis5_w7_apply_runs_chunked(&chunked, runs, 8, 2u);

    const unsigned int sig_direct = axis5_w7_sig(&direct);
    const unsigned int sig_chunked = axis5_w7_sig(&chunked);
    const unsigned int same = (sig_direct == sig_chunked) ? 1u : 0u;

    printf("%u %u %u\n", sig_direct, sig_chunked, same);
    return 0;
}
