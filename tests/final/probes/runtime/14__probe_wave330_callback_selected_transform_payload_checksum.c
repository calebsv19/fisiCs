#include <stdio.h>

typedef struct Atom {
    unsigned char id;
    union {
        struct {
            unsigned char x;
            unsigned char y;
        };
        unsigned short pair;
        unsigned char raw[2];
    } data;
} Atom;

typedef struct Bundle {
    unsigned char stamp;
    Atom atoms[5];
    unsigned char seal;
} Bundle;

typedef Bundle (*Transform)(Bundle, unsigned char);

static Bundle make_bundle(unsigned char seed) {
    Bundle out;
    unsigned i;

    out.stamp = (unsigned char)(0x40u + seed);
    for (i = 0u; i < 5u; ++i) {
        out.atoms[i].id = (unsigned char)(seed + 3u + i * 6u);
        out.atoms[i].data.raw[0] = (unsigned char)(0x50u + seed + i * 4u);
        out.atoms[i].data.raw[1] = (unsigned char)(0x80u + seed + i * 9u);
    }
    out.seal = (unsigned char)(0xA0u + seed);
    return out;
}

static Bundle bump_even(Bundle in, unsigned char salt) {
    Bundle out = in;
    unsigned i;

    for (i = 0u; i < 5u; i += 2u) {
        out.atoms[i].data.x = (unsigned char)(out.atoms[i].data.x + salt + i);
        out.atoms[i].data.y ^= (unsigned char)(salt + out.atoms[i].id);
    }
    out.seal = (unsigned char)(out.seal + out.atoms[4].data.raw[0]);
    return out;
}

static Bundle swap_edges(Bundle in, unsigned char salt) {
    Bundle out = in;
    Atom saved = out.atoms[0];

    out.atoms[0] = out.atoms[4];
    out.atoms[4] = saved;
    out.atoms[2].data.pair = (unsigned short)(out.atoms[2].data.pair + (unsigned short)(salt * 17u));
    out.stamp = (unsigned char)(out.stamp ^ out.atoms[2].data.raw[1]);
    return out;
}

static Transform choose_transform(unsigned index) {
    Transform table[2];

    table[0] = bump_even;
    table[1] = swap_edges;
    return table[index & 1u];
}

static unsigned fold_bundle(Bundle value) {
    unsigned acc = (unsigned)value.stamp * 241u + (unsigned)value.seal;
    unsigned i;

    for (i = 0u; i < 5u; ++i) {
        acc = acc * 127u + (unsigned)value.atoms[i].id;
        acc = acc * 127u + (unsigned)value.atoms[i].data.raw[0];
        acc = acc * 127u + (unsigned)value.atoms[i].data.raw[1];
    }
    return acc;
}

int main(void) {
    Bundle value = make_bundle(4u);
    Transform op;
    unsigned i;
    unsigned typed = 0u;

    for (i = 0u; i < 4u; ++i) {
        op = choose_transform(i + 1u);
        value = op(value, (unsigned char)(0x13u + i * 7u));
    }

    typed += (unsigned)value.stamp + (unsigned)value.seal;
    typed += (unsigned)value.atoms[0].id * 3u + (unsigned)value.atoms[0].data.x * 5u;
    typed += (unsigned)value.atoms[2].data.y * 7u + (unsigned)value.atoms[3].data.raw[0] * 11u;
    typed += (unsigned)value.atoms[4].id * 13u + (unsigned)value.atoms[4].data.raw[1] * 17u;

    printf("%u %u %u %u %u %u %u %u %u %u\n",
           (unsigned)value.stamp,
           (unsigned)value.atoms[0].id,
           (unsigned)value.atoms[0].data.raw[0],
           (unsigned)value.atoms[1].data.raw[1],
           (unsigned)value.atoms[2].data.raw[0],
           (unsigned)value.atoms[3].id,
           (unsigned)value.atoms[4].data.raw[1],
           (unsigned)value.seal,
           typed,
           fold_bundle(value));
    return 0;
}
