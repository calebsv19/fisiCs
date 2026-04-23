#include <stdio.h>

void ow62_reset(void);
void ow62_enable(unsigned lane);
void ow62_disable(unsigned lane);
unsigned ow62_emit(unsigned lane, unsigned payload);
int ow62_consume(unsigned lane, unsigned token, unsigned* out_value);
unsigned ow62_digest(void);

int main(void) {
    unsigned i;
    unsigned ok = 0u;
    unsigned drop = 0u;
    unsigned value = 0u;
    unsigned digest = 0u;

    ow62_reset();

    for (i = 0u; i < 72u; ++i) {
        unsigned lane = (i * 7u + 3u) & 3u;
        unsigned token;

        if ((i % 4u) != 0u) {
            ow62_enable(lane);
        }
        if ((i % 6u) == 1u) {
            ow62_disable((lane + 1u) & 3u);
        }

        token = ow62_emit(lane, i * 23u + 11u);

        if (ow62_consume(lane, token, &value)) {
            ok += 1u;
            digest = (digest * 149u) ^ value;
        } else {
            drop += 1u;
        }

        if (ow62_consume(lane, token - 1u, &value)) {
            ok += 1u;
            digest = (digest * 149u) ^ value;
        } else {
            drop += 1u;
        }

        if ((i % 5u) == 0u) {
            ow62_disable(lane);
        }
    }

    printf("%u %u %u\n", ok, drop, ow62_digest() ^ digest);
    return 0;
}
