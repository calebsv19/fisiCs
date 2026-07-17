union Wave128Bits {
    unsigned words[4];
    struct {
        unsigned lo;
        unsigned hi;
        unsigned tag;
        unsigned mask;
    } named;
};

struct Wave128Leaf {
    unsigned id;
    long bias;
    union Wave128Bits bits;
};

struct Wave128Payload {
    unsigned epoch;
    struct Wave128Leaf lanes[3][2];
    union {
        struct {
            unsigned route;
            unsigned salt;
        } named;
        unsigned words[2];
    } meta;
};

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

static struct Wave128Leaf make_leaf(unsigned seed, long bias, unsigned row, unsigned col) {
    struct Wave128Leaf leaf;
    unsigned base = seed + row * 53u + col * 97u + (unsigned)(bias * 7L);

    leaf.id = rotl32(base ^ 0x7f4a7c15u, row * 5u + col + 3u);
    leaf.bias = bias + (long)(row * 11u) - (long)(col * 13u);
    leaf.bits.named.lo = base * 17u + 5u;
    leaf.bits.named.hi = rotl32(base + 0x45d9f3bu, row + col + 9u);
    leaf.bits.named.tag = leaf.id ^ (unsigned)(leaf.bias * 31L);
    leaf.bits.named.mask = leaf.bits.named.lo ^ leaf.bits.named.hi ^ leaf.bits.named.tag;
    return leaf;
}

struct Wave128Payload wave128_make_payload(unsigned seed, long bias) {
    struct Wave128Payload payload;
    unsigned row;
    unsigned col;

    payload.epoch = seed ^ rotl32((unsigned)(bias * 29L + 113L), seed & 31u);
    payload.meta.named.route = seed % 3u;
    payload.meta.named.salt = seed * 2654435761u + (unsigned)(bias * 101L + 37L);

    for (row = 0u; row < 3u; ++row) {
        for (col = 0u; col < 2u; ++col) {
            payload.lanes[row][col] = make_leaf(seed + row * 11u, bias, row, col);
        }
    }

    return payload;
}

struct Wave128Payload wave128_mix_payload(struct Wave128Payload payload, unsigned step) {
    unsigned row;
    unsigned col;

    payload.epoch ^= rotl32(step + payload.meta.words[1], step & 31u);
    payload.meta.named.route = (payload.meta.named.route + step) % 3u;
    payload.meta.named.salt ^= rotl32(step * 17u + payload.epoch, (step & 7u) + 3u);

    for (row = 0u; row < 3u; ++row) {
        for (col = 0u; col < 2u; ++col) {
            struct Wave128Leaf leaf = payload.lanes[row][col];
            unsigned turn = step + row * 3u + col;
            leaf.id ^= rotl32(payload.meta.words[col], turn & 31u);
            leaf.bias += (long)((int)(turn & 7u) - 3);
            leaf.bits.words[(row + col) & 3u] ^= leaf.id + (unsigned)leaf.bias + turn;
            leaf.bits.named.mask = leaf.bits.named.lo ^ rotl32(leaf.bits.named.hi, col + 5u) ^ leaf.bits.named.tag;
            payload.lanes[(row + step) % 3u][col] = leaf;
        }
    }

    return payload;
}

unsigned wave128_fold_payload(struct Wave128Payload payload, unsigned salt) {
    unsigned acc = salt ^ payload.epoch ^ payload.meta.words[0] ^ payload.meta.words[1];
    unsigned row;
    unsigned col;

    for (row = 0u; row < 3u; ++row) {
        for (col = 0u; col < 2u; ++col) {
            struct Wave128Leaf leaf = payload.lanes[(row + payload.meta.named.route) % 3u][col];
            acc ^= leaf.id + (unsigned)(leaf.bias * 131L);
            acc = rotl32(acc, row * 5u + col + 7u);
            acc += leaf.bits.words[(row + col) & 3u] ^ leaf.bits.named.mask;
            acc ^= rotl32(leaf.bits.named.lo + leaf.bits.named.hi + leaf.bits.named.tag, row + col + 11u);
        }
    }

    return acc ^ (acc >> 16u);
}
