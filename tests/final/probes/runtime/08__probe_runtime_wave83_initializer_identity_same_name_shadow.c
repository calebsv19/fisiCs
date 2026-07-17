#include <stdio.h>

union Value {
    unsigned int word;
    unsigned int parts[2];
};

typedef struct Bundle {
    unsigned int marker;
    union Value lanes[2];
    unsigned int seal;
} Bundle;

static unsigned int outer_checksum(const struct Bundle *bundle) {
    return bundle->marker * 3u + bundle->lanes[0].parts[0] * 5u +
           bundle->lanes[0].parts[1] * 7u +
           bundle->lanes[1].word * 11u + bundle->seal * 13u;
}

int main(void) {
    Bundle outer = {
        .marker = 3u,
        .lanes = {
            [0] = {.parts = {[0] = 5u, [1] = 7u}},
            [1] = {.word = 11u},
        },
        .seal = 13u,
    };
    unsigned int inner_checksum;
    unsigned int inner_size;

    {
        union InnerValue {
            unsigned int word;
            unsigned int parts[2];
        };
        typedef struct Bundle {
            union InnerValue lanes[3];
            unsigned int marker;
            unsigned int seal;
        } Bundle;
        Bundle inner = {
            .lanes = {
                [0] = {.parts = {[0] = 17u, [1] = 19u}},
                [1] = {.word = 23u},
                [2] = {.parts = {[1] = 29u}},
            },
            .marker = 31u,
            .seal = 37u,
        };

        inner_checksum = inner.lanes[0].parts[0] * 17u +
                         inner.lanes[0].parts[1] * 19u +
                         inner.lanes[1].word * 23u +
                         inner.lanes[2].parts[0] * 29u +
                         inner.lanes[2].parts[1] * 31u +
                         inner.marker * 37u + inner.seal * 41u;
        inner_size = (unsigned int)sizeof(Bundle);
    }

    printf("%u %u %u\n", outer_checksum(&outer) * 2u + inner_checksum * 5u,
           (unsigned int)sizeof(Bundle), inner_size);
    return 0;
}
