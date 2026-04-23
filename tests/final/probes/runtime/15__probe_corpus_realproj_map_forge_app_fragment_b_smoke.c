#include <stdio.h>
#include <stddef.h>

typedef struct {
    const char* line;
} Raw;

static int parse_uint(const char* s, unsigned* out) {
    unsigned v = 0u;
    if (*s == '\0') {
        return 0;
    }
    while (*s != '\0') {
        char c = *s;
        if (c < '0' || c > '9') {
            return 0;
        }
        v = v * 10u + (unsigned)(c - '0');
        ++s;
    }
    *out = v;
    return 1;
}

int main(void) {
    static const Raw rows[] = {
        {"z=12"},
        {"x=655"},
        {"y=1583"},
        {"gen=2"},
        {"tile_count=9"},
        {"tile_count=11"},
        {"gen=oops"},
        {"x=700"},
    };
    unsigned i;
    unsigned digest = 2166136261u;
    unsigned applied = 0u;

    for (i = 0u; i < (unsigned)(sizeof(rows) / sizeof(rows[0])); ++i) {
        const char* line = rows[i].line;
        const char* eq = line;
        unsigned value = 0u;
        unsigned lane = 0u;

        while (*eq != '\0' && *eq != '=') {
            ++eq;
        }
        if (*eq != '=') {
            continue;
        }
        if (!parse_uint(eq + 1, &value)) {
            continue;
        }

        if (line[0] == 'z') {
            lane = value * 17u;
        } else if (line[0] == 'x') {
            lane = value * 23u + 5u;
        } else if (line[0] == 'y') {
            lane = value * 29u + 7u;
        } else if (line[0] == 'g') {
            lane = value * 31u + 11u;
        } else if (line[0] == 't') {
            lane = value * 37u + 13u;
        } else {
            continue;
        }

        digest ^= lane;
        digest *= 16777619u;
        digest ^= (digest >> 15);
        applied += 1u;
    }

    printf("%u %u\n", applied, digest);
    return 0;
}
