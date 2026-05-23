#include <stdio.h>

static unsigned alias_feedback(unsigned seed, unsigned delta) {
    static unsigned window[8] = {3u, 5u, 8u, 13u, 21u, 34u, 55u, 89u};
    static unsigned epoch = 2u;
    unsigned acc = seed ^ (epoch * 17u);
    unsigned i;

    for (i = 0u; i < 8u; ++i) {
        window[i] = (window[i] + delta + i * 3u + epoch) % 257u;
        acc = acc * 131u + window[i];
    }

    {
        unsigned char* raw = (unsigned char*)&window[(seed + epoch) & 3u];
        raw[0] = (unsigned char)(raw[0] ^ (unsigned char)(delta * 5u + 1u));
        raw[1] = (unsigned char)(raw[1] + (unsigned char)(epoch + 3u));
    }

    epoch = (epoch + delta + (acc & 7u)) % 29u;
    for (i = 0u; i < 4u; ++i) {
        acc = acc * 137u + window[(i + epoch) & 7u];
    }
    return acc ^ (epoch * 97u);
}

int main(void) {
    unsigned x = alias_feedback(9u, 4u);
    unsigned y = alias_feedback(13u, 7u);
    unsigned z = alias_feedback(21u, 3u);
    printf("%u %u %u\n", x, y, z);
    return 0;
}
