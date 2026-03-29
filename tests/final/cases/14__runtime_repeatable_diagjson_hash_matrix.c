#include <stdio.h>

struct DiagRec {
    unsigned code;
    unsigned line;
    unsigned column;
    unsigned kind;
};

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

static int build_diag_model(unsigned mode, struct DiagRec out[8]) {
    int count = 0;
    unsigned i;
    for (i = 0; i < 3u; ++i) {
        out[count].code = 1000u + mode * 13u + i;
        out[count].line = 4u + i * 3u + mode;
        out[count].column = 2u + (mode ^ i);
        out[count].kind = (i + mode) & 1u;
        ++count;
    }
    if ((mode & 1u) == 0u) {
        out[count].code = 2000u + mode;
        out[count].line = 40u + mode;
        out[count].column = 1u;
        out[count].kind = 0u;
        ++count;
    } else {
        out[count].code = 3000u + mode;
        out[count].line = 90u - mode;
        out[count].column = 7u;
        out[count].kind = 1u;
        ++count;
    }
    return count;
}

static unsigned hash_diag_records(const struct DiagRec recs[8], int count) {
    unsigned h = 2166136261u;
    int i;
    for (i = 0; i < count; ++i) {
        h = fnv1a_u32(h, recs[i].code);
        h = fnv1a_u32(h, recs[i].line);
        h = fnv1a_u32(h, recs[i].column);
        h = fnv1a_u32(h, recs[i].kind);
    }
    return h;
}

int main(void) {
    unsigned matrix_hash = 2166136261u;
    unsigned shape_hash = 2166136261u;
    unsigned mode;

    for (mode = 0u; mode < 6u; ++mode) {
        struct DiagRec a[8];
        struct DiagRec b[8];
        int ca = build_diag_model(mode, a);
        int cb = build_diag_model(mode, b);
        unsigned ha = hash_diag_records(a, ca);
        unsigned hb = hash_diag_records(b, cb);
        if (ca != cb || ha != hb) {
            printf("diag-repeatability-fail %u %d %d %u %u\n", mode, ca, cb, ha, hb);
            return 17;
        }
        matrix_hash = fnv1a_u32(matrix_hash, ha);
        shape_hash = fnv1a_u32(shape_hash, (unsigned)ca * 100u + mode);
    }

    printf("%u %u\n", matrix_hash, shape_hash);
    return 0;
}
