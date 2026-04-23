#include <stdio.h>

typedef struct Axis5W1Step {
    unsigned int op;
    int payload;
} Axis5W1Step;

static unsigned int axis5_w1_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x85ebca6bu + (h << 7) + (h >> 3);
    return h;
}

static int axis5_w1_minimize_once(const Axis5W1Step* in, int n, Axis5W1Step* out) {
    int m = 0;
    for (int i = 0; i < n; ++i) {
        Axis5W1Step cur = in[i];
        if (m > 0 && out[m - 1].op == cur.op && out[m - 1].payload == cur.payload) {
            continue;
        }
        if (m > 0 && out[m - 1].op == cur.op && (cur.op & 1u) == 0u) {
            out[m - 1].payload += cur.payload;
            continue;
        }
        out[m++] = cur;
    }
    return m;
}

static unsigned int axis5_w1_signature(const Axis5W1Step* seq, int n) {
    unsigned int h = 0x811c9dc5u;
    for (int i = 0; i < n; ++i) {
        h = axis5_w1_mix(h, seq[i].op & 15u);
        h = axis5_w1_mix(h, (unsigned int)(seq[i].payload & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W1Step raw[] = {
        {2u, 5}, {2u, 7}, {2u, 7}, {1u, 9}, {1u, 9}, {4u, 3},
        {4u, 8}, {5u, 2}, {5u, 2}, {8u, 1}, {8u, 4}, {8u, 4},
    };
    Axis5W1Step pass1[16];
    Axis5W1Step pass2[16];

    const int n1 = axis5_w1_minimize_once(raw, 12, pass1);
    const int n2 = axis5_w1_minimize_once(pass1, n1, pass2);

    const unsigned int s1 = axis5_w1_signature(pass1, n1);
    const unsigned int s2 = axis5_w1_signature(pass2, n2);
    const unsigned int idem = (n1 == n2 && s1 == s2) ? 1u : 0u;

    printf("%d %d %u %u %u\n", n1, n2, s1, s2, idem);
    return 0;
}
