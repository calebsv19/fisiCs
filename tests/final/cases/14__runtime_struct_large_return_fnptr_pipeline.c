#include <stdio.h>

typedef struct Frame {
    long long lane[3];
    int tag;
} Frame;

typedef Frame (*BuildFn)(int);

static Frame make_add(int seed) {
    Frame f;
    f.lane[0] = (long long)seed + 10;
    f.lane[1] = (long long)seed + 20;
    f.lane[2] = (long long)seed + 30;
    f.tag = seed * 2;
    return f;
}

static Frame make_mul(int seed) {
    Frame f;
    f.lane[0] = (long long)seed * 3;
    f.lane[1] = (long long)seed * 5;
    f.lane[2] = (long long)seed * 7;
    f.tag = seed * 4;
    return f;
}

static Frame mix(Frame a, Frame b) {
    Frame out;
    out.lane[0] = a.lane[1] + b.lane[2];
    out.lane[1] = a.lane[2] - b.lane[0];
    out.lane[2] = a.lane[0] + b.lane[1];
    out.tag = a.tag ^ b.tag;
    return out;
}

static long long score(Frame f) {
    return f.lane[0] * 2 + f.lane[1] * 3 + f.lane[2] * 5 + (long long)f.tag;
}

int main(void) {
    BuildFn builders[2] = {make_add, make_mul};
    Frame cache[4];
    for (int i = 0; i < 4; ++i) {
        cache[i] = builders[i & 1](i + 2);
    }

    Frame acc = make_add(1);
    for (int i = 0; i < 7; ++i) {
        Frame cur = (i & 1) ? builders[(i + 1) & 1](i + 3) : cache[i & 3];
        acc = mix(acc, cur);
    }

    printf("%lld %lld\n", score(acc), score(cache[2]));
    return 0;
}

