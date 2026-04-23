#include <stdio.h>

typedef struct Axis5W2Row {
    unsigned int lane;
    unsigned int key;
    unsigned int value;
} Axis5W2Row;

static void axis5_w2_swap(Axis5W2Row* a, Axis5W2Row* b) {
    Axis5W2Row t = *a;
    *a = *b;
    *b = t;
}

static int axis5_w2_less(const Axis5W2Row* a, const Axis5W2Row* b) {
    if (a->lane != b->lane) return a->lane < b->lane;
    if (a->key != b->key) return a->key < b->key;
    return a->value < b->value;
}

static void axis5_w2_sort_rows(Axis5W2Row* rows, int n) {
    for (int i = 1; i < n; ++i) {
        int j = i;
        while (j > 0 && axis5_w2_less(&rows[j], &rows[j - 1])) {
            axis5_w2_swap(&rows[j], &rows[j - 1]);
            --j;
        }
    }
}

static unsigned int axis5_w2_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static unsigned int axis5_w2_canonical_signature(Axis5W2Row* rows, int n) {
    axis5_w2_sort_rows(rows, n);
    unsigned int h = 2166136261u;
    for (int i = 0; i < n; ++i) {
        h = axis5_w2_mix(h, rows[i].lane & 0xfu);
        h = axis5_w2_mix(h, rows[i].key & 0x3ffu);
        h = axis5_w2_mix(h, rows[i].value & 0xffffu);
    }
    return h;
}

int main(void) {
    Axis5W2Row perm_a[] = {
        {3u, 42u, 11u}, {1u, 9u, 7u}, {2u, 18u, 99u}, {1u, 4u, 8u},
        {0u, 77u, 5u}, {3u, 8u, 17u}, {2u, 18u, 3u}, {0u, 9u, 4u},
    };
    Axis5W2Row perm_b[] = {
        {0u, 9u, 4u}, {3u, 8u, 17u}, {1u, 4u, 8u}, {2u, 18u, 99u},
        {1u, 9u, 7u}, {0u, 77u, 5u}, {3u, 42u, 11u}, {2u, 18u, 3u},
    };

    const unsigned int sig_a = axis5_w2_canonical_signature(perm_a, 8);
    const unsigned int sig_b = axis5_w2_canonical_signature(perm_b, 8);
    const unsigned int same = (sig_a == sig_b) ? 1u : 0u;

    printf("%u %u %u\n", sig_a, sig_b, same);
    return 0;
}
