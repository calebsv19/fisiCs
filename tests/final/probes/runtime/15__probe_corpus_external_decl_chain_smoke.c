#include <stdio.h>
#include <stddef.h>

typedef struct {
    const char *decl;
} DeclLine;

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

static int parse_decl(const char *line, char *kind, size_t kcap, char *type, size_t tcap, char *name, size_t ncap) {
    const char *p = line;
    size_t ki = 0, ti = 0, ni = 0;
    int field = 0;

    while (*p != '\0') {
        if (*p == ':') {
            ++field;
            ++p;
            continue;
        }
        if (field == 0) {
            if (ki + 1 < kcap) {
                kind[ki++] = *p;
            }
        } else if (field == 1) {
            if (ti + 1 < tcap) {
                type[ti++] = *p;
            }
        } else if (field == 2) {
            if (ni + 1 < ncap) {
                name[ni++] = *p;
            }
        } else {
            return 0;
        }
        ++p;
    }
    kind[ki] = '\0';
    type[ti] = '\0';
    name[ni] = '\0';
    return ki > 0 && ti > 0 && ni > 0 && field == 2;
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

static unsigned chain_depth(const char *name) {
    unsigned depth = 1u;
    while (*name != '\0') {
        if (*name == '.') {
            ++depth;
        }
        ++name;
    }
    return depth;
}

int main(void) {
    static const DeclLine fragment[] = {
        {"extern:int:session.user.id"},
        {"static:uint:cache.bucket.count"},
        {"auto:short:route.node.weight"},
        {"extern:char:meta.tag"},
        {"broken_decl"},
        {"volatile:long:epoch.window.tick"},
        {"register:uint:state.path.depth"},
    };

    unsigned acc = 0x9e3779b9u;
    unsigned accepted = 0u;
    size_t i;

    for (i = 0; i < sizeof(fragment) / sizeof(fragment[0]); ++i) {
        char kind[16];
        char type[16];
        char name[64];
        int w;
        unsigned lane;

        if (!parse_decl(fragment[i].decl, kind, sizeof(kind), type, sizeof(type), name, sizeof(name))) {
            continue;
        }
        w = type_width(type);
        if (w == 0) {
            continue;
        }
        lane = hash_ci(kind) ^ hash_ci(name) ^ (unsigned)(w * (int)chain_depth(name));
        acc ^= lane + accepted * 131u + (unsigned)(i * 17u);
        acc = (acc << 7u) | (acc >> 25u);
        acc ^= (acc >> 11u);
        ++accepted;
    }

    printf("%u %u\n", accepted, acc);
    return 0;
}
