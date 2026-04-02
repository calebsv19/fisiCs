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

static int split_decl(const char *line, char *type_buf, size_t type_cap, char *name_buf, size_t name_cap) {
    const char *p = line;
    size_t ti = 0;
    size_t ni = 0;

    while (*p != '\0' && *p != ':') {
        if (ti + 1 < type_cap) {
            type_buf[ti++] = *p;
        }
        ++p;
    }
    if (*p != ':') {
        return 0;
    }
    ++p;
    while (*p != '\0') {
        if (ni + 1 < name_cap) {
            name_buf[ni++] = *p;
        }
        ++p;
    }
    type_buf[ti] = '\0';
    name_buf[ni] = '\0';
    return ti > 0 && ni > 0;
}

int main(void) {
    static const DeclLine fragment[] = {
        {"int:id"},
        {"uint:flags"},
        {"char:tag"},
        {"long:epoch"},
        {"short:code"},
        {"broken_decl"},
        {"uint:mask"},
    };

    unsigned acc = 0x9e3779b9u;
    unsigned accepted = 0u;
    size_t i;

    for (i = 0; i < sizeof(fragment) / sizeof(fragment[0]); ++i) {
        char type_buf[16];
        char name_buf[32];
        int width;
        unsigned lane;

        if (!split_decl(fragment[i].decl, type_buf, sizeof(type_buf), name_buf, sizeof(name_buf))) {
            continue;
        }
        width = type_width(type_buf);
        if (width == 0) {
            continue;
        }
        lane = key_hash(name_buf) ^ (unsigned)(width * (int)(i + 3));
        acc ^= lane + (unsigned)(accepted * 131u);
        acc = (acc << 7) | (acc >> 25);
        acc ^= (acc >> 11);
        ++accepted;
    }

    printf("%u %u\n", accepted, acc);
    return 0;
}
