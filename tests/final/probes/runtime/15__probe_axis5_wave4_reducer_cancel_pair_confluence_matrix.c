#include <stdio.h>

typedef struct Axis5W4Event {
    unsigned int key;
    int delta;
} Axis5W4Event;

typedef struct Axis5W4Pair {
    unsigned int key;
    int delta;
} Axis5W4Pair;

static unsigned int axis5_w4_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static int axis5_w4_push_cancel_ltr(
    const Axis5W4Event* in,
    int n,
    Axis5W4Pair* out
) {
    int m = 0;
    for (int i = 0; i < n; ++i) {
        Axis5W4Pair cur = {in[i].key, in[i].delta};
        if (m > 0 && out[m - 1].key == cur.key && out[m - 1].delta == -cur.delta) {
            --m;
            continue;
        }
        out[m++] = cur;
    }
    return m;
}

static int axis5_w4_push_cancel_rtl(
    const Axis5W4Event* in,
    int n,
    Axis5W4Pair* out
) {
    int m = 0;
    for (int i = n - 1; i >= 0; --i) {
        Axis5W4Pair cur = {in[i].key, in[i].delta};
        if (m > 0 && out[m - 1].key == cur.key && out[m - 1].delta == -cur.delta) {
            --m;
            continue;
        }
        out[m++] = cur;
    }

    for (int i = 0; i < m / 2; ++i) {
        Axis5W4Pair t = out[i];
        out[i] = out[m - 1 - i];
        out[m - 1 - i] = t;
    }
    return m;
}

static unsigned int axis5_w4_normal_form_sig(const Axis5W4Pair* seq, int n) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < n; ++i) {
        h = axis5_w4_mix(h, seq[i].key & 31u);
        h = axis5_w4_mix(h, (unsigned int)(seq[i].delta & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W4Event stream[] = {
        {2u, +5}, {1u, +3}, {1u, -3}, {4u, +7}, {4u, -7}, {2u, -5},
        {7u, +2}, {7u, -2}, {5u, +9}, {5u, -9}, {3u, +4}, {3u, -4},
        {6u, +8}, {6u, -8}, {0u, +1}, {0u, -1},
    };
    Axis5W4Pair ltr[16];
    Axis5W4Pair rtl[16];

    const int n_ltr = axis5_w4_push_cancel_ltr(stream, 16, ltr);
    const int n_rtl = axis5_w4_push_cancel_rtl(stream, 16, rtl);
    const unsigned int sig_ltr = axis5_w4_normal_form_sig(ltr, n_ltr);
    const unsigned int sig_rtl = axis5_w4_normal_form_sig(rtl, n_rtl);
    const unsigned int same = (n_ltr == n_rtl && sig_ltr == sig_rtl) ? 1u : 0u;

    printf("%d %d %u %u %u\n", n_ltr, n_rtl, sig_ltr, sig_rtl, same);
    return 0;
}
