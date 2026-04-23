#include <stddef.h>

static unsigned g_paws_state = 0u;

void paws_init(unsigned* buf, size_t n) {
    size_t i;
    for (i = 0u; i < n; ++i) {
        buf[i] = 11u + (unsigned)i * 7u;
    }
    g_paws_state = 29u + (unsigned)n;
}

unsigned paws_window_step(unsigned* buf, size_t n, size_t start, size_t width, unsigned delta) {
    unsigned fold = delta * 31u + 3u;
    size_t limit = start + width;
    size_t i;

    if (start >= n) {
        return g_paws_state;
    }
    if (limit > n) {
        limit = n;
    }

    for (i = start; i < limit; ++i) {
        buf[i] = buf[i] + delta + (unsigned)(i - start) * 5u;
        fold = fold * 137u + buf[i];
    }

    if (limit > start) {
        size_t mid = start + (limit - start) / 2u;
        unsigned char* raw = (unsigned char*)&buf[mid];
        raw[0] = (unsigned char)(raw[0] ^ (unsigned char)(delta * 3u + 1u));
        fold = fold * 149u + buf[mid];
    }

    g_paws_state = g_paws_state * 167u + fold;
    return fold;
}

unsigned paws_state(void) {
    return g_paws_state;
}
