#include <stdio.h>

void ter_reset(void);
unsigned ter_issue(unsigned token, unsigned* out_epoch);
int ter_accept(unsigned slot, unsigned epoch, unsigned* out_value);
void ter_reseed(unsigned slot, unsigned seed);
unsigned ter_digest(void);

int main(void) {
    unsigned i;
    unsigned first_slot = 0u;
    unsigned first_epoch = 0u;
    unsigned value = 0u;
    unsigned accepted = 0u;
    unsigned dropped = 0u;
    unsigned digest = 29u;

    ter_reset();

    for (i = 0u; i < 40u; ++i) {
        unsigned epoch = 0u;
        unsigned slot = ter_issue(100u + i * 3u, &epoch);
        if (i == 2u) {
            first_slot = slot;
            first_epoch = epoch;
        }
    }

    if (ter_accept(first_slot, first_epoch, &value)) {
        accepted += 1u;
        digest = digest * 149u + value;
    } else {
        dropped += 1u;
    }

    ter_reseed(first_slot, 77u);
    if (ter_accept(first_slot, first_epoch, &value)) {
        accepted += 1u;
        digest = digest * 149u + value;
    } else {
        dropped += 1u;
    }

    if (ter_accept(first_slot, (first_epoch + 1u) % 16u, &value)) {
        accepted += 1u;
        digest = digest * 149u + value;
    } else {
        dropped += 1u;
    }

    printf("%u %u %u %u\n", accepted, dropped, digest, ter_digest());
    return 0;
}
