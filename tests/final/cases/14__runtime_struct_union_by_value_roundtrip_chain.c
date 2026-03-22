#include <stdio.h>

typedef union Payload {
    long long ll;
    int pair[2];
} Payload;

typedef struct Outer {
    int tag;
    Payload p;
    double scale;
} Outer;

static Outer make_pair(int tag, int x, int y, double s) {
    Outer o;
    o.tag = tag | 1;
    o.p.pair[0] = x;
    o.p.pair[1] = y;
    o.scale = s;
    return o;
}

static Outer make_ll(int tag, long long v, double s) {
    Outer o;
    o.tag = tag & ~1;
    o.p.ll = v;
    o.scale = s;
    return o;
}

static Outer twist(Outer o, int k) {
    if (o.tag & 1) {
        o.p.pair[0] += k;
        o.p.pair[1] -= k * 2;
    } else {
        o.p.ll += (long long)k * 111LL;
    }
    o.scale += (double)k * 0.5;
    o.tag ^= (k << 1);
    return o;
}

static Outer mix(Outer a, Outer b) {
    Outer out;
    if (a.tag & 1) {
        int bx = (b.tag & 1) ? b.p.pair[0] : (int)(b.p.ll & 0x7fff);
        int by = (b.tag & 1) ? b.p.pair[1] : (int)((b.p.ll >> 16) & 0x7fff);
        out = make_pair(a.tag + b.tag, a.p.pair[0] + bx, a.p.pair[1] - by, a.scale + b.scale);
    } else {
        long long bv = (b.tag & 1) ? (long long)b.p.pair[0] * 4096LL + (long long)b.p.pair[1] : b.p.ll;
        out = make_ll(a.tag - b.tag, a.p.ll - bv + 17LL, a.scale * 2.0 + b.scale);
    }
    return out;
}

static long long score(Outer o) {
    long long core = (o.tag & 1)
        ? (long long)o.p.pair[0] * 31LL + (long long)o.p.pair[1] * 17LL
        : o.p.ll;
    return core + (long long)o.tag * 13LL + (long long)(o.scale * 100.0);
}

int main(void) {
    Outer a = make_pair(3, 40, 9, 1.5);
    Outer b = make_ll(8, 900000123LL, 2.25);
    Outer c = twist(a, 5);
    Outer d = twist(b, 3);
    Outer e = mix(c, d);
    Outer f = mix(twist(e, 2), c);
    printf("%lld %lld\n", score(e), score(f));
    return 0;
}

