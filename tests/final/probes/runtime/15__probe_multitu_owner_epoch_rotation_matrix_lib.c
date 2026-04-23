#define OW62_LANES 4u

typedef struct {
    unsigned epoch;
    unsigned value;
    unsigned releases;
    int active;
} ow62_lane;

static ow62_lane g_lane[OW62_LANES];

void ow62_reset(void) {
    unsigned i;
    for (i = 0u; i < OW62_LANES; ++i) {
        g_lane[i].epoch = 0u;
        g_lane[i].value = 0u;
        g_lane[i].releases = 0u;
        g_lane[i].active = 0;
    }
}

void ow62_enable(unsigned lane) {
    ow62_lane* s = &g_lane[lane & 3u];
    if (!s->active) {
        s->active = 1;
        s->epoch += 1u;
    }
}

void ow62_disable(unsigned lane) {
    ow62_lane* s = &g_lane[lane & 3u];
    if (s->active) {
        s->active = 0;
        s->releases += 1u;
    }
}

unsigned ow62_emit(unsigned lane, unsigned payload) {
    ow62_lane* s = &g_lane[lane & 3u];
    if (!s->active) {
        return 0u;
    }
    s->value = (payload * 43u) ^ (s->epoch * 19u + lane * 7u + s->releases * 5u);
    return (s->epoch << 2u) | (lane & 3u);
}

int ow62_consume(unsigned lane, unsigned token, unsigned* out_value) {
    ow62_lane* s = &g_lane[lane & 3u];
    unsigned token_lane = token & 3u;
    unsigned token_epoch = token >> 2u;

    if (token == 0u) {
        return 0;
    }
    if (token_lane != (lane & 3u)) {
        return 0;
    }
    if (!s->active || s->epoch != token_epoch) {
        return 0;
    }
    if (out_value) {
        *out_value = s->value;
    }
    return 1;
}

unsigned ow62_digest(void) {
    unsigned i;
    unsigned digest = 37u;
    for (i = 0u; i < OW62_LANES; ++i) {
        digest = (digest * 173u)
            ^ (g_lane[i].epoch * 13u)
            ^ (g_lane[i].value * 11u)
            ^ (g_lane[i].releases * 7u)
            ^ ((unsigned)g_lane[i].active * 5u)
            ^ i;
    }
    return digest;
}
