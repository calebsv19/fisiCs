#include <stdio.h>

typedef struct Mix {
    float f;
    double d;
    int i;
    long long k;
} Mix;

static Mix step(Mix a, Mix b, double s, float t, int u, long long v) {
    Mix out;
    out.f = (a.f + b.f) * t;
    out.d = (a.d - b.d) * s;
    out.i = a.i + b.i + u;
    out.k = a.k - b.k + v;
    return out;
}

static long long score(Mix m) {
    long long sf = (long long)(m.f * 100.0f);
    long long sd = (long long)(m.d * 100.0);
    return sf * 3 + sd * 5 + (long long)m.i * 7 + m.k * 11;
}

int main(void) {
    Mix a = {1.25f, 4.5, 3, 100};
    Mix b = {2.5f, 1.25, 7, -40};
    Mix c = step(a, b, 2.0, 1.5f, 5, 13);
    Mix d = step(c, a, 0.5, 2.0f, -3, 29);

    printf("%lld %lld\n", score(c), score(d));
    return 0;
}
