#include <stdio.h>
#include <stddef.h>

typedef struct {
    const char *line;
} EdgeLine;

static int parse_uint10(const char *z, int *out_value, int *consumed) {
    int v = 0;
    int n = 0;
    if (*z < '0' || *z > '9') {
        return 0;
    }
    while (*z >= '0' && *z <= '9') {
        v = v * 10 + (*z - '0');
        ++z;
        ++n;
    }
    *out_value = v;
    *consumed = n;
    return 1;
}

static int parse_edge(const char *line, int *src, int *dst, int *w) {
    int n0 = 0;
    int n1 = 0;
    int n2 = 0;
    int a = 0;
    int b = 0;
    int c = 0;

    if (!parse_uint10(line, &a, &n0)) {
        return 0;
    }
    line += n0;
    if (*line != '-') {
        return 0;
    }
    ++line;
    if (!parse_uint10(line, &b, &n1)) {
        return 0;
    }
    line += n1;
    if (*line != ':') {
        return 0;
    }
    ++line;
    if (!parse_uint10(line, &c, &n2)) {
        return 0;
    }
    line += n2;
    if (*line != '\0') {
        return 0;
    }

    *src = a;
    *dst = b;
    *w = c;
    return 1;
}

int main(void) {
    static const EdgeLine rows[] = {
        {"0-1:7"},
        {"1-2:11"},
        {"2-3:13"},
        {"3-4:17"},
        {"4-0:19"},
        {"bad"},
        {"2-4:23"},
        {"1-3:29"}
    };

    unsigned acc = 0x9e3779b9u;
    unsigned accepted = 0u;

    for (size_t i = 0u; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        int src = 0;
        int dst = 0;
        int w = 0;
        unsigned lane;

        if (!parse_edge(rows[i].line, &src, &dst, &w)) {
            continue;
        }
        lane = (unsigned)(src * 97 + dst * 131 + w * 17);
        acc ^= lane + (unsigned)i * 19u + accepted * 23u;
        acc = (acc << 7u) | (acc >> 25u);
        acc ^= (acc >> 13u);
        ++accepted;
    }

    printf("%u %u\n", accepted, acc);
    return 0;
}
