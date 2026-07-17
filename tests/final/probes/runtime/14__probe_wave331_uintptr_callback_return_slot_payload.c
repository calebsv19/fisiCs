#include <stdint.h>
#include <stdio.h>

typedef struct Token {
    uintptr_t route;
    union {
        struct {
            unsigned char low;
            unsigned char mid;
            unsigned char high;
        };
        unsigned char raw[3];
    } payload;
} Token;

typedef Token (*TokenOp)(Token, uintptr_t);

static Token make_token(uintptr_t base, unsigned char seed) {
    Token out;

    out.route = base + (uintptr_t)seed * 17u;
    out.payload.raw[0] = (unsigned char)(0x15u + seed);
    out.payload.raw[1] = (unsigned char)(0x35u + seed * 2u);
    out.payload.raw[2] = (unsigned char)(0x65u + seed * 3u);
    return out;
}

static Token bump_low(Token in, uintptr_t salt) {
    Token out = in;

    out.payload.low = (unsigned char)(out.payload.low + (unsigned char)(salt & 31u));
    out.route += (uintptr_t)out.payload.high;
    return out;
}

static Token rotate_high(Token in, uintptr_t salt) {
    Token out = in;
    unsigned char saved = out.payload.raw[0];

    out.payload.raw[0] = out.payload.raw[2];
    out.payload.raw[2] = (unsigned char)(saved ^ (unsigned char)(salt & 63u));
    out.payload.mid = (unsigned char)(out.payload.mid + out.payload.raw[2]);
    out.route ^= ((uintptr_t)out.payload.mid << 4);
    return out;
}

static TokenOp choose_op(uintptr_t route) {
    TokenOp table[2];

    table[0] = bump_low;
    table[1] = rotate_high;
    return table[(unsigned)(route >> 3) & 1u];
}

static unsigned fold_token(Token value) {
    unsigned acc = (unsigned)(value.route & 0xffffu);
    unsigned i;

    for (i = 0u; i < 3u; ++i) {
        acc = acc * 149u + (unsigned)value.payload.raw[i] + i;
    }
    return acc;
}

int main(void) {
    Token slots[3];
    uintptr_t base = (uintptr_t)0x4300u;
    unsigned i;
    unsigned typed = 0u;

    for (i = 0u; i < 3u; ++i) {
        Token value = make_token(base + (uintptr_t)i * 5u, (unsigned char)(8u + i));
        TokenOp op = choose_op(value.route);
        slots[i] = op(value, (uintptr_t)(0x23u + i * 11u));
    }

    slots[2] = choose_op(slots[0].route)(slots[2], slots[1].route);
    typed += (unsigned)(slots[0].route & 0xffffu) + (unsigned)(slots[1].route & 0xffffu);
    typed += (unsigned)slots[0].payload.raw[0] * 3u + (unsigned)slots[1].payload.mid * 5u;
    typed += (unsigned)slots[2].payload.high * 7u;

    printf("%u %u %u %u %u %u %u %u\n",
           (unsigned)(slots[0].route & 0xffffu),
           (unsigned)slots[0].payload.raw[0],
           (unsigned)slots[0].payload.raw[2],
           (unsigned)(slots[1].route & 0xffffu),
           (unsigned)slots[1].payload.mid,
           (unsigned)slots[2].payload.high,
           typed,
           fold_token(slots[2]));
    return 0;
}
