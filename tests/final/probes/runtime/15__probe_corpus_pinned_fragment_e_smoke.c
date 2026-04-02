#include <stdio.h>
#include <stddef.h>

typedef struct {
    const char *line;
} CorpusLine;

static int ascii_eq(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int parse_field(const char *line, int field, char *buf, size_t cap) {
    const char *p = line;
    int cur = 0;
    size_t n = 0;

    while (*p != '\0') {
        if (cur == field) {
            if (*p == ':') {
                break;
            }
            if (n + 1 < cap) {
                buf[n++] = *p;
            }
        }
        if (*p == ':') {
            ++cur;
        }
        ++p;
    }

    if (cur < field) {
        return 0;
    }
    buf[n] = '\0';
    return n > 0;
}

static int parse_uint10(const char *z, int *out) {
    int value = 0;
    if (*z == '\0') {
        return 0;
    }
    while (*z != '\0') {
        if (*z < '0' || *z > '9') {
            return 0;
        }
        value = value * 10 + (*z - '0');
        ++z;
    }
    *out = value;
    return 1;
}

static int width_of(const char *ty) {
    if (ascii_eq(ty, "char")) {
        return 1;
    }
    if (ascii_eq(ty, "short")) {
        return 2;
    }
    if (ascii_eq(ty, "int")) {
        return 4;
    }
    if (ascii_eq(ty, "long")) {
        return 8;
    }
    return 0;
}

int main(void) {
    static const CorpusLine rows[] = {
        {"typedef:int:NodeId:4"},
        {"typedef:long:Tick:8"},
        {"typedef:char:Tag:1"},
        {"typedef:short:Code:2"},
        {"bad_line"},
        {"typedef:int:Mask:4"},
        {"typedef:float:Skip:4"}
    };

    unsigned acc = 2166136261u;
    unsigned accepted = 0u;

    for (size_t i = 0u; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        char k0[16];
        char k1[16];
        char k2[32];
        char k3[16];
        int expect_w = 0;
        int actual_w = 0;
        unsigned lane;

        if (!parse_field(rows[i].line, 0, k0, sizeof(k0))) {
            continue;
        }
        if (!parse_field(rows[i].line, 1, k1, sizeof(k1))) {
            continue;
        }
        if (!parse_field(rows[i].line, 2, k2, sizeof(k2))) {
            continue;
        }
        if (!parse_field(rows[i].line, 3, k3, sizeof(k3))) {
            continue;
        }
        if (!ascii_eq(k0, "typedef")) {
            continue;
        }
        if (!parse_uint10(k3, &expect_w)) {
            continue;
        }
        actual_w = width_of(k1);
        if (actual_w == 0 || actual_w != expect_w) {
            continue;
        }

        lane = (unsigned)(expect_w * (int)(i + 11u));
        for (const char *p = k2; *p != '\0'; ++p) {
            lane = (lane * 131u) ^ (unsigned char)*p;
        }
        acc ^= lane + accepted * 97u;
        acc *= 16777619u;
        ++accepted;
    }

    printf("%u %u\n", accepted, acc);
    return 0;
}
