struct HugeState {
    long long lane[6];
    int tag;
    double scale;
};

struct HugeState abi_huge_make(int seed, long long base) {
    struct HugeState out;
    for (int i = 0; i < 6; ++i) {
        out.lane[i] = base + (long long)(seed + i) * (long long)(i + 2);
    }
    out.tag = seed * 5;
    out.scale = 1.0 + (double)seed * 0.25;
    return out;
}

struct HugeState abi_huge_twist(struct HugeState in, int k) {
    for (int i = 0; i < 6; ++i) {
        in.lane[i] += (long long)k * (long long)(i + 1);
    }
    in.tag += k * 3;
    in.scale += (double)k * 0.5;
    return in;
}

struct HugeState abi_huge_blend(struct HugeState a, struct HugeState b, int lane) {
    struct HugeState out;
    for (int i = 0; i < 6; ++i) {
        out.lane[i] = a.lane[5 - i] + b.lane[i] - (long long)(lane + i) * 7LL;
    }
    out.tag = (a.tag ^ b.tag) + lane;
    out.scale = a.scale * 0.75 + b.scale * 1.25 + (double)lane;
    return out;
}

long long abi_huge_score(struct HugeState in) {
    long long score = 0;
    for (int i = 0; i < 6; ++i) {
        score += in.lane[i] * (long long)(i + 3);
    }
    score += (long long)in.tag * 17LL;
    score += (long long)(in.scale * 100.0);
    return score;
}
