#include <stddef.h>

static unsigned g_state = 0u;
static unsigned g_epoch = 0u;

void ssarm_init(unsigned* buf, size_t n, unsigned salt) {
    size_t i;
    for (i = 0u; i < n; ++i) {
        buf[i] = 17u + (unsigned)i * 9u + salt * 3u;
    }
    g_state = 29u + salt * 11u + (unsigned)n;
    g_epoch = 7u + salt * 5u;
}

unsigned ssarm_step(unsigned* buf, size_t n, size_t start, size_t width, unsigned delta) {
    unsigned fold = g_state ^ (delta * 31u) ^ (g_epoch * 17u);
    size_t limit = start + width;
    size_t i;

    if (start >= n) {
        return g_state;
    }
    if (limit > n) {
        limit = n;
    }

    for (i = start; i < limit; ++i) {
        buf[i] = buf[i] + delta + (unsigned)(i - start) * 7u + (g_epoch & 3u);
        fold = fold * 137u + buf[i];
    }

    if (limit > start) {
        size_t mid = start + (limit - start) / 2u;
        unsigned char* raw = (unsigned char*)&buf[mid];
        raw[0] = (unsigned char)(raw[0] ^ (unsigned char)(delta * 5u + 3u));
        raw[1] = (unsigned char)(raw[1] + (unsigned char)(g_epoch + 1u));
        fold = fold * 149u + buf[mid];
    }

    g_epoch = (g_epoch + delta + (fold & 7u) + (unsigned)(limit - start)) % 257u;
    g_state = g_state * 167u + fold + g_epoch;
    return fold ^ (g_state * 19u);
}

unsigned ssarm_state(void) {
    return g_state ^ (g_epoch * 97u);
}
