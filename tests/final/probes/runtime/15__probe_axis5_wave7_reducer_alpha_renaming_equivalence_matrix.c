#include <stdio.h>

typedef struct Axis5W7Pair {
    unsigned int key;
    unsigned int value;
} Axis5W7Pair;

static unsigned int axis5_w7_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w7_swap(unsigned int* a, unsigned int* b) {
    unsigned int t = *a;
    *a = *b;
    *b = t;
}

static void axis5_w7_sort(unsigned int* v, int n) {
    for (int i = 1; i < n; ++i) {
        int j = i;
        while (j > 0 && v[j - 1] > v[j]) {
            axis5_w7_swap(&v[j - 1], &v[j]);
            --j;
        }
    }
}

static int axis5_w7_reduce_by_key(const Axis5W7Pair* in, int n, unsigned int* out_values) {
    unsigned int bins[8] = {0u};
    for (int i = 0; i < n; ++i) bins[in[i].key & 7u] += in[i].value & 0xffffu;
    int m = 0;
    for (int i = 0; i < 8; ++i) {
        if (bins[i] == 0u) continue;
        out_values[m++] = bins[i];
    }
    axis5_w7_sort(out_values, m);
    return m;
}

static unsigned int axis5_w7_multiset_sig(const unsigned int* values, int n) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < n; ++i) h = axis5_w7_mix(h, values[i]);
    return h;
}

int main(void) {
    const Axis5W7Pair original[] = {
        {0u, 7u}, {1u, 3u}, {2u, 8u}, {0u, 5u}, {3u, 2u}, {1u, 9u},
        {2u, 4u}, {3u, 6u}, {0u, 1u}, {1u, 2u},
    };
    const Axis5W7Pair renamed[] = {
        {5u, 7u}, {7u, 3u}, {1u, 8u}, {5u, 5u}, {2u, 2u}, {7u, 9u},
        {1u, 4u}, {2u, 6u}, {5u, 1u}, {7u, 2u},
    };
    unsigned int a[8];
    unsigned int b[8];

    const int n_a = axis5_w7_reduce_by_key(original, 10, a);
    const int n_b = axis5_w7_reduce_by_key(renamed, 10, b);
    const unsigned int sig_a = axis5_w7_multiset_sig(a, n_a);
    const unsigned int sig_b = axis5_w7_multiset_sig(b, n_b);
    const unsigned int same = (n_a == n_b && sig_a == sig_b) ? 1u : 0u;

    printf("%d %d %u %u %u\n", n_a, n_b, sig_a, sig_b, same);
    return 0;
}
