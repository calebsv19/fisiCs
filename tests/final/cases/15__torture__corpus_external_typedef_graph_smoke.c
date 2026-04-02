#include <stdio.h>
#include <stddef.h>

typedef struct {
    const char *line;
} TypedefLine;

static unsigned hash_ci(const char *z) {
    unsigned h = 2166136261u;
    while (*z != '\0') {
        unsigned ch = (unsigned char)*z;
        if (ch >= 'A' && ch <= 'Z') {
            ch = ch - 'A' + 'a';
        }
        h ^= ch;
        h *= 16777619u;
        ++z;
    }
    return h;
}

static int parse_edge(
    const char *line,
    char *alias,
    size_t alias_cap,
    char *base,
    size_t base_cap,
    unsigned *depth
) {
    const char *p = line;
    size_t ai = 0;
    size_t bi = 0;
    unsigned d = 0;
    int phase = 0;

    while (*p != '\0') {
        if (phase == 0) {
            if (*p == '=') {
                phase = 1;
                ++p;
                continue;
            }
            if (ai + 1 < alias_cap) {
                alias[ai++] = *p;
            }
            ++p;
            continue;
        }
        if (phase == 1) {
            if (*p == '*') {
                phase = 2;
                ++p;
                continue;
            }
            if (bi + 1 < base_cap) {
                base[bi++] = *p;
            }
            ++p;
            continue;
        }
        if (*p < '0' || *p > '9') {
            return 0;
        }
        d = d * 10u + (unsigned)(*p - '0');
        ++p;
    }

    alias[ai] = '\0';
    base[bi] = '\0';
    *depth = d;
    return ai > 0 && bi > 0 && phase == 2;
}

static int base_weight(const char *base) {
    if (base[0] == 'i' && base[1] == 'n' && base[2] == 't' && base[3] == '\0') {
        return 4;
    }
    if (base[0] == 'u' && base[1] == 'i' && base[2] == 'n' && base[3] == 't' && base[4] == '\0') {
        return 5;
    }
    if (base[0] == 's' && base[1] == 'h' && base[2] == 'o' && base[3] == 'r' && base[4] == 't' && base[5] == '\0') {
        return 3;
    }
    if (base[0] == 'l' && base[1] == 'o' && base[2] == 'n' && base[3] == 'g' && base[4] == '\0') {
        return 7;
    }
    return 0;
}

int main(void) {
    static const TypedefLine graph[] = {
        {"Node=int*1"},
        {"NodeList=uint*2"},
        {"RouteKey=short*3"},
        {"bad_line"},
        {"Epoch=long*2"},
        {"Delta=int*4"},
        {"Spare=void*2"},
    };

    unsigned acc = 0xa511e9b3u;
    unsigned accepted = 0u;
    size_t i;

    for (i = 0; i < sizeof(graph) / sizeof(graph[0]); ++i) {
        char alias[32];
        char base[32];
        unsigned depth = 0u;
        int w;
        unsigned lane;

        if (!parse_edge(graph[i].line, alias, sizeof(alias), base, sizeof(base), &depth)) {
            continue;
        }
        w = base_weight(base);
        if (w == 0 || depth == 0u) {
            continue;
        }
        lane = hash_ci(alias) ^ hash_ci(base) ^ (unsigned)(w * (int)depth + (int)(i * 23u + 5u));
        acc ^= lane + accepted * 109u;
        acc = (acc << 5u) | (acc >> 27u);
        acc ^= (acc >> 9u);
        ++accepted;
    }

    printf("%u %u\n", accepted, acc);
    return 0;
}
