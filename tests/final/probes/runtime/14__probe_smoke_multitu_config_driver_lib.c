struct Config {
    int base;
    int stride;
    int mode;
};

typedef int (*ModeFn)(int seed, int x);

static int mode_add(int seed, int x) { return seed + x + 11; }
static int mode_fold(int seed, int x) { return (seed * (x + 3)) % 8191; }
static int mode_twist(int seed, int x) { return (seed ^ (x * 17 + 23)) + 19; }

static ModeFn select_mode(int mode) {
    switch (mode & 3) {
        case 0: return mode_add;
        case 1: return mode_fold;
        default: return mode_twist;
    }
}

const struct Config *fetch_config(int idx) {
    static const struct Config table[] = {
        {7, 2, 0},
        {11, 3, 1},
        {5, 5, 2},
        {13, 1, 1}
    };
    int n = (int)(sizeof(table) / sizeof(table[0]));
    if (idx < 0) {
        idx = -idx;
    }
    return &table[idx % n];
}

int apply_config(const struct Config *cfg, int x) {
    ModeFn fn = select_mode(cfg->mode);
    int seed = cfg->base + cfg->stride * x;
    return fn(seed, x + cfg->mode);
}

int fold_config_trace(int acc, int i, int value) {
    return acc + value * (i + 1) + i * 7;
}
