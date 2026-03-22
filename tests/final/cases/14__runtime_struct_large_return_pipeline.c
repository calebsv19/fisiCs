#include <stdio.h>

typedef struct Packet {
    long long lane[4];
    int bias;
} Packet;

static Packet build_packet(int seed) {
    Packet p;
    for (int i = 0; i < 4; ++i) {
        p.lane[i] = (long long)seed * (long long)(i + 2) + (long long)i;
    }
    p.bias = seed * 3;
    return p;
}

static Packet twist_packet(Packet p, int k) {
    for (int i = 0; i < 4; ++i) {
        p.lane[i] += (long long)k * (long long)(i + 1);
    }
    p.bias += k * 2;
    return p;
}

static Packet merge_packet(Packet a, Packet b) {
    Packet out;
    for (int i = 0; i < 4; ++i) {
        out.lane[i] = a.lane[i] - b.lane[3 - i] + (long long)(i * 3);
    }
    out.bias = a.bias - b.bias;
    return out;
}

static long long packet_score(Packet p) {
    long long score = 0;
    for (int i = 0; i < 4; ++i) {
        score += p.lane[i] * (long long)(i + 1);
    }
    score += (long long)p.bias * 11LL;
    return score;
}

int main(void) {
    Packet a = build_packet(4);
    Packet b = build_packet(9);
    a = twist_packet(a, 3);
    b = twist_packet(b, 5);
    Packet m = merge_packet(a, b);
    Packet n = merge_packet(twist_packet(m, 2), a);

    printf("%lld %lld\n", packet_score(m), packet_score(n));
    return 0;
}

