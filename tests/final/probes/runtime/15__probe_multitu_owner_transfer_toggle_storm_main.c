#include <stdio.h>

void ots_reset(void);
void ots_toggle(unsigned slot, int enabled);
int ots_transfer(unsigned slot, unsigned from, unsigned to);
int ots_release(unsigned slot, unsigned owner, unsigned* out_value);
unsigned ots_digest(void);

int main(void) {
    unsigned i;
    unsigned released = 0u;
    unsigned blocked = 0u;
    unsigned out = 0u;
    unsigned digest = 67u;

    ots_reset();

    for (i = 0u; i < 3u; ++i) {
        ots_toggle(i, 1);
    }
    for (i = 0u; i < 9u; ++i) {
        unsigned slot = i % 3u;
        unsigned from = (i + slot) & 3u;
        unsigned to = (from + 1u) & 3u;
        if (ots_transfer(slot, from, to)) {
            digest = digest * 181u + slot * 7u + to;
        } else {
            digest = digest * 181u + slot * 5u + from;
            blocked += 1u;
        }
        if ((i % 4u) == 3u) {
            ots_toggle(slot, 0);
            ots_toggle(slot, 1);
        }
    }

    for (i = 0u; i < 3u; ++i) {
        if (ots_release(i, (i + 1u) & 3u, &out)) {
            released += 1u;
            digest = digest * 181u + out;
        } else {
            blocked += 1u;
        }
    }

    printf("%u %u %u %u\n", released, blocked, digest, ots_digest());
    return 0;
}
