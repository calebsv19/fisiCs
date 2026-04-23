#include <stdio.h>

static unsigned fnv1a_u32(unsigned h, unsigned v) {
    h ^= (v & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 8) & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 16) & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 24) & 0xFFu);
    h *= 16777619u;
    return h;
}

static unsigned hash_text(unsigned h, const char* text) {
    const unsigned char* p = (const unsigned char*)text;
    while (*p != '\0') {
        h ^= (unsigned)*p++;
        h *= 16777619u;
    }
    return h;
}

static unsigned run_text_matrix(unsigned seed) {
    unsigned h = 2166136261u;
    unsigned acc = seed * 97u + 13u;
    unsigned row;

    for (row = 0u; row < 12u; ++row) {
        char line[96];
        unsigned left = (acc ^ (row * 131u + 7u)) + (seed * 3u);
        unsigned right = (acc * (row + 5u)) ^ (seed * 17u + row);
        int n = snprintf(line, sizeof(line), "row=%u left=%u right=%u", row, left, right);

        acc = fnv1a_u32(acc, left ^ right ^ (unsigned)n);
        h = hash_text(h, line);
    }

    return fnv1a_u32(h, acc ^ seed);
}

int main(void) {
    unsigned agg = 2166136261u;
    unsigned cross = 0u;
    unsigned seed;

    for (seed = 5u; seed < 14u; ++seed) {
        unsigned a = run_text_matrix(seed);
        unsigned b = run_text_matrix(seed);
        if (a != b) {
            printf("repeatability-fail %u %u %u\n", seed, a, b);
            return 9;
        }
        agg = fnv1a_u32(agg, a ^ (seed * 19u));
        cross ^= (a + seed * 23u);
    }

    printf("%u %u\n", agg, cross);
    return 0;
}
