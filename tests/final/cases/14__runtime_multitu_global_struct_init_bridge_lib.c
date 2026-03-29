struct GlobalCfg {
    int a;
    long long b;
    double c;
    int arr[3];
};

struct GlobalCfg g_global_cfg = {4, 900LL, 1.5, {2, 4, 6}};

struct GlobalCfg abi_global_cfg_current(void) {
    return g_global_cfg;
}

struct GlobalCfg abi_global_cfg_tick(int k) {
    g_global_cfg.a += k;
    g_global_cfg.b += (long long)k * 11LL;
    g_global_cfg.c += (double)k * 0.25;
    for (int i = 0; i < 3; ++i) {
        g_global_cfg.arr[i] += k * (i + 1);
    }
    return g_global_cfg;
}

long long abi_global_cfg_score(struct GlobalCfg cfg) {
    long long score = 0;
    score += (long long)cfg.a * 13LL;
    score += cfg.b;
    score += (long long)(cfg.c * 100.0);
    score += (long long)cfg.arr[0] + (long long)cfg.arr[1] * 3LL + (long long)cfg.arr[2] * 5LL;
    return score;
}
