struct MixPayload {
    long long big;
    int small;
};

long long abi_mix_bridge(
    int a,
    unsigned b,
    long long c,
    double d,
    struct MixPayload payload,
    unsigned short u
) {
    long long scaled = (long long)(d * 10.0);
    return c +
           payload.big +
           (long long)payload.small +
           scaled +
           (long long)u +
           (long long)a -
           (long long)b;
}
