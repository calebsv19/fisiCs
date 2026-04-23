#include <stdio.h>

void tw62_reset(unsigned seed);
unsigned tw62_issue(unsigned slot);
int tw62_write(unsigned slot, unsigned token, unsigned value);
int tw62_read(unsigned slot, unsigned token, unsigned* out_value);
void tw62_rotate(unsigned slot, unsigned salt);
unsigned tw62_digest(void);

int main(void) {
    unsigned i;
    unsigned ok = 0u;
    unsigned drop = 0u;
    unsigned read_value = 0u;
    unsigned digest = 0u;

    tw62_reset(37u);

    for (i = 0u; i < 64u; ++i) {
        unsigned slot = (i * 5u + 1u) & 7u;
        unsigned token = tw62_issue(slot);
        unsigned stale = token;

        if ((i % 3u) == 0u) {
            tw62_rotate(slot, i + 9u);
        }

        if (tw62_write(slot, token, i * 29u + 7u)) {
            ok += 1u;
        } else {
            drop += 1u;
        }

        if (tw62_read(slot, stale, &read_value)) {
            ok += 1u;
            digest = (digest * 167u) ^ read_value;
        } else {
            drop += 1u;
        }

        if ((i % 5u) == 2u) {
            tw62_rotate((slot + 3u) & 7u, i + 17u);
        }
    }

    printf("%u %u %u\n", ok, drop, tw62_digest() ^ digest);
    return 0;
}
