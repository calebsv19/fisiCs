#include <stdio.h>

typedef struct BigState {
    long long lane[5];
    int tag;
} BigState;

static BigState build_state(int seed) {
    BigState s;
    for (int i = 0; i < 5; ++i) {
        s.lane[i] = (long long)(seed + i) * (long long)(i + 2);
    }
    s.tag = seed * 3;
    return s;
}

static BigState blend_state(BigState a, BigState b, int k) {
    BigState out;
    for (int i = 0; i < 5; ++i) {
        out.lane[i] = a.lane[4 - i] + b.lane[i] - (long long)k * (long long)(i + 1);
    }
    out.tag = (a.tag ^ b.tag) + k;
    return out;
}

static long long score_state(BigState s) {
    long long score = 0;
    for (int i = 0; i < 5; ++i) {
        score += s.lane[i] * (long long)(i + 1);
    }
    score += (long long)s.tag * 13LL;
    return score;
}

int main(void) {
    BigState a = build_state(3);
    BigState b = build_state(8);
    BigState m = blend_state(a, b, 2);
    BigState n = (m.tag & 1) ? blend_state(m, a, 3) : blend_state(b, m, 4);
    printf("%lld %lld\n", score_state(m), score_state(n));
    return 0;
}
