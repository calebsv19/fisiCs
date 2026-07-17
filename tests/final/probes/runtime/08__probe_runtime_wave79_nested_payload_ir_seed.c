#include <stddef.h>
#include <stdio.h>

union Atom {
    unsigned char bytes[4];
    int scalar;
};

struct Pair {
    unsigned char tag;
    union Atom atom;
};

struct Envelope {
    struct Pair rows[2][2];
    unsigned char end;
};

static struct Pair choose_pair(int selector) {
    return selector ? (struct Pair){ .tag = 17, .atom.bytes = { 19, 23, 0, 29 } }
                    : (struct Pair){ .tag = 31, .atom.bytes = { [2] = 37 } };
}

static unsigned checksum(struct Envelope envelope) {
    unsigned acc = envelope.end;

    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const struct Pair *pair = &envelope.rows[r][c];
            acc = acc * 53u + pair->tag;
            acc = acc * 47u + pair->atom.bytes[0];
            acc = acc * 43u + pair->atom.bytes[2];
            acc = acc * 41u + pair->atom.bytes[3];
        }
    }

    return acc;
}

int main(void) {
    struct Envelope envelope = {
        .rows[0][0] = { .tag = 3, .atom.bytes = { 5, 7, 11 } },
        .rows[1][1].atom.bytes[1] = 13,
        .end = 41,
    };

    envelope.rows[0][1] = choose_pair(1);
    envelope.rows[1][0] = (struct Pair){ .tag = 43, .atom = (union Atom){ .bytes = { [0] = 47, [3] = 53 } } };
    envelope.rows[1][1].tag = 59;

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Envelope, end),
           (unsigned)envelope.rows[0][1].atom.bytes[2],
           (unsigned)envelope.rows[1][0].atom.bytes[1],
           (unsigned)envelope.rows[1][0].atom.bytes[3],
           (unsigned)envelope.rows[1][1].atom.bytes[1],
           checksum(envelope));
    return 0;
}
