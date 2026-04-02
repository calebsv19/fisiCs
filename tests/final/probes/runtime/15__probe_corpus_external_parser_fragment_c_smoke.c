#include <stdio.h>
#include <stddef.h>

typedef struct {
    const char *decl;
} DeclLine;

static unsigned key_hash(const char *z) {
    unsigned h = 2166136261u;
    while (*z != '\0') {
        h ^= (unsigned char)*z;
        h *= 16777619u;
        ++z;
    }
    return h;
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

static int qual_weight(const char *q) {
    if (q[0] == 's' && q[1] == 't' && q[2] == 'a' && q[3] == 't' && q[4] == 'i' && q[5] == 'c' && q[6] == '\0') {
        return 3;
    }
    if (q[0] == 'e' && q[1] == 'x' && q[2] == 't' && q[3] == 'e' && q[4] == 'r' && q[5] == 'n' && q[6] == '\0') {
        return 5;
    }
    if (q[0] == 'a' && q[1] == 'u' && q[2] == 't' && q[3] == 'o' && q[4] == '\0') {
        return 7;
    }
    if (q[0] == 'r' && q[1] == 'e' && q[2] == 'g' && q[3] == 'i' && q[4] == 's' && q[5] == 't' && q[6] == 'e' && q[7] == 'r' && q[8] == '\0') {
        return 11;
    }
    if (q[0] == 'v' && q[1] == 'o' && q[2] == 'l' && q[3] == 'a' && q[4] == 't' && q[5] == 'i' && q[6] == 'l' && q[7] == 'e' && q[8] == '\0') {
        return 13;
    }
    return 0;
}

static int type_width(const char *z) {
    if (z[0] == 'i' && z[1] == 'n' && z[2] == 't' && z[3] == '\0') {
        return 4;
    }
    if (z[0] == 'c' && z[1] == 'h' && z[2] == 'a' && z[3] == 'r' && z[4] == '\0') {
        return 1;
    }
    if (z[0] == 's' && z[1] == 'h' && z[2] == 'o' && z[3] == 'r' && z[4] == 't' && z[5] == '\0') {
        return 2;
    }
    if (z[0] == 'l' && z[1] == 'o' && z[2] == 'n' && z[3] == 'g' && z[4] == '\0') {
        return 8;
    }
    if (z[0] == 'u' && z[1] == 'i' && z[2] == 'n' && z[3] == 't' && z[4] == '\0') {
        return 4;
    }
    return 0;
}

int main(void) {
    static const DeclLine fragment[] = {
        {"static:int:counter"},
        {"extern:uint:flags"},
        {"auto:short:code"},
        {"register:char:tag"},
        {"broken_decl"},
        {"volatile:long:epoch"},
        {"extern:uint:mask"},
    };

    unsigned acc = 0x7f4a7c15u;
    unsigned accepted = 0u;
    size_t i;

    for (i = 0; i < sizeof(fragment) / sizeof(fragment[0]); ++i) {
        char q_buf[16];
        char t_buf[16];
        char n_buf[32];
        int q;
        int w;
        unsigned lane;

        if (!parse_field(fragment[i].decl, 0, q_buf, sizeof(q_buf))) {
            continue;
        }
        if (!parse_field(fragment[i].decl, 1, t_buf, sizeof(t_buf))) {
            continue;
        }
        if (!parse_field(fragment[i].decl, 2, n_buf, sizeof(n_buf))) {
            continue;
        }

        q = qual_weight(q_buf);
        w = type_width(t_buf);
        if (q == 0 || w == 0) {
            continue;
        }
        lane = key_hash(n_buf) ^ (unsigned)(q * w + (int)i * 17 + 19);
        acc ^= lane + accepted * 131u;
        acc = (acc << 9) | (acc >> 23);
        acc ^= (acc >> 13);
        ++accepted;
    }

    printf("%u %u\n", accepted, acc);
    return 0;
}
