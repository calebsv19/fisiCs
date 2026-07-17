#include <stdio.h>

typedef struct Payload {
    unsigned char tag;
    struct {
        union {
            struct {
                unsigned char x;
                unsigned char y;
                unsigned char z;
            };
            unsigned char bytes[3];
        };
    } nested;
    unsigned char guard;
} Payload;

typedef struct Store {
    unsigned short epoch;
    Payload slots[3];
    unsigned short checksum_seed;
} Store;

typedef Payload (*PayloadMaker)(unsigned char, unsigned char);

static Payload make_alpha(unsigned char seed, unsigned char salt) {
    Payload out;

    out.tag = (unsigned char)(0x20u + seed);
    out.nested.bytes[0] = (unsigned char)(seed + salt);
    out.nested.bytes[1] = (unsigned char)(seed ^ salt);
    out.nested.bytes[2] = (unsigned char)(0xC0u - seed);
    out.guard = (unsigned char)(out.nested.x + out.nested.z);
    return out;
}

static Payload make_beta(unsigned char seed, unsigned char salt) {
    Payload out;

    out.tag = (unsigned char)(0x60u + salt);
    out.nested.x = (unsigned char)(seed * 3u + 1u);
    out.nested.y = (unsigned char)(salt * 5u + 2u);
    out.nested.z = (unsigned char)(out.nested.x ^ out.nested.y ^ 0x5Au);
    out.guard = (unsigned char)(out.tag + out.nested.y);
    return out;
}

static unsigned hash_store(Store value) {
    unsigned acc = (unsigned)value.epoch * 149u + (unsigned)value.checksum_seed;
    unsigned i;
    unsigned j;

    for (i = 0u; i < 3u; ++i) {
        acc = acc * 181u + (unsigned)value.slots[i].tag;
        for (j = 0u; j < 3u; ++j) {
            acc = acc * 113u + (unsigned)value.slots[i].nested.bytes[j] + i + j;
        }
        acc = acc * 181u + (unsigned)value.slots[i].guard;
    }
    return acc;
}

static Store fill_store(PayloadMaker makers[2], unsigned char seed) {
    Store out;
    unsigned i;

    out.epoch = (unsigned short)(0x2200u + seed);
    out.checksum_seed = (unsigned short)(0x4400u + seed * 3u);
    for (i = 0u; i < 3u; ++i) {
        Payload current = makers[i & 1u]((unsigned char)(seed + i * 6u), (unsigned char)(0x13u + i * 9u));
        out.slots[(i + 1u) % 3u] = current;
    }
    out.slots[0].nested.bytes[2] ^= out.slots[2].tag;
    out.slots[1].guard = (unsigned char)(out.slots[1].guard + out.slots[0].nested.x);
    return out;
}

int main(void) {
    PayloadMaker makers[2];
    Store store;
    unsigned typed = 0u;

    makers[0] = make_alpha;
    makers[1] = make_beta;
    store = fill_store(makers, 7u);

    typed += (unsigned)store.epoch + (unsigned)store.checksum_seed;
    typed += (unsigned)store.slots[0].tag * 3u + (unsigned)store.slots[0].nested.z * 5u;
    typed += (unsigned)store.slots[1].tag * 7u + (unsigned)store.slots[1].nested.y * 11u;
    typed += (unsigned)store.slots[2].tag * 13u + (unsigned)store.slots[2].guard * 17u;

    printf("%u %u %u %u %u %u %u %u %u %u\n",
           (unsigned)store.epoch,
           (unsigned)store.slots[0].tag,
           (unsigned)store.slots[0].nested.bytes[0],
           (unsigned)store.slots[0].nested.bytes[2],
           (unsigned)store.slots[1].tag,
           (unsigned)store.slots[1].guard,
           (unsigned)store.slots[2].tag,
           (unsigned)store.slots[2].nested.bytes[1],
           typed,
           hash_store(store));
    return 0;
}
