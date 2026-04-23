#include <stdio.h>

typedef struct Axis5W5Pair {
    unsigned int key;
    int value;
} Axis5W5Pair;

static unsigned int axis5_w5_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w5_swap(Axis5W5Pair* a, Axis5W5Pair* b) {
    Axis5W5Pair t = *a;
    *a = *b;
    *b = t;
}

static void axis5_w5_sort(Axis5W5Pair* rows, int n) {
    for (int i = 1; i < n; ++i) {
        int j = i;
        while (j > 0 && rows[j - 1].key > rows[j].key) {
            axis5_w5_swap(&rows[j - 1], &rows[j]);
            --j;
        }
    }
}

static int axis5_w5_pipeline_sort_then_collapse(
    const Axis5W5Pair* in,
    int n,
    Axis5W5Pair* out
) {
    Axis5W5Pair tmp[16];
    for (int i = 0; i < n; ++i) tmp[i] = in[i];
    axis5_w5_sort(tmp, n);

    int m = 0;
    for (int i = 0; i < n; ++i) {
        if (m > 0 && out[m - 1].key == tmp[i].key) {
            out[m - 1].value += tmp[i].value;
        } else {
            out[m++] = tmp[i];
        }
    }

    int k = 0;
    for (int i = 0; i < m; ++i) {
        if (out[i].value != 0) out[k++] = out[i];
    }
    return k;
}

static int axis5_w5_pipeline_histogram(
    const Axis5W5Pair* in,
    int n,
    Axis5W5Pair* out
) {
    int bins[8] = {0};
    for (int i = 0; i < n; ++i) bins[in[i].key & 7u] += in[i].value;

    int m = 0;
    for (unsigned int key = 0; key < 8u; ++key) {
        if (bins[key] == 0) continue;
        out[m].key = key;
        out[m].value = bins[key];
        ++m;
    }
    return m;
}

static unsigned int axis5_w5_sig(const Axis5W5Pair* rows, int n) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < n; ++i) {
        h = axis5_w5_mix(h, rows[i].key & 31u);
        h = axis5_w5_mix(h, (unsigned int)(rows[i].value & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W5Pair raw[] = {
        {3u, +7}, {1u, +5}, {3u, -2}, {0u, +4}, {1u, -1}, {7u, +3},
        {0u, -4}, {2u, +6}, {7u, +2}, {2u, -6}, {1u, +9}, {3u, -5},
    };
    Axis5W5Pair p_a[16];
    Axis5W5Pair p_b[16];

    const int n_a = axis5_w5_pipeline_sort_then_collapse(raw, 12, p_a);
    const int n_b = axis5_w5_pipeline_histogram(raw, 12, p_b);
    const unsigned int sig_a = axis5_w5_sig(p_a, n_a);
    const unsigned int sig_b = axis5_w5_sig(p_b, n_b);
    const unsigned int same = (n_a == n_b && sig_a == sig_b) ? 1u : 0u;

    printf("%d %d %u %u %u\n", n_a, n_b, sig_a, sig_b, same);
    return 0;
}
