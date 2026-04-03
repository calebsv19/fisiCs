typedef struct {
    const char *name;
    unsigned weight;
} PassInfo;

static const PassInfo k_passes[] = {
    {"cfg", 3u},
    {"sroa", 5u},
    {"dce", 2u}
};

const PassInfo *registry_pick(unsigned idx) {
    unsigned n = (unsigned)(sizeof(k_passes) / sizeof(k_passes[0]));
    if (idx >= n) return (const PassInfo *)0;
    return &k_passes[idx];
}

unsigned registry_count(void) {
    return (unsigned)(sizeof(k_passes) / sizeof(k_passes[0]));
}
