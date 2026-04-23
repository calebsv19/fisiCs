#include <stdio.h>

void tg14_init(void);
int tg14_enqueue(unsigned value, unsigned* out_slot, unsigned* out_generation);
int tg14_accept(unsigned slot, unsigned generation, unsigned* out_value);
void tg14_force_reuse(unsigned slot, unsigned value);
unsigned tg14_state_digest(void);

int main(void) {
    unsigned slot0 = 0u;
    unsigned gen0 = 0u;
    unsigned slot1 = 0u;
    unsigned gen1 = 0u;
    unsigned value = 0u;
    unsigned accepted = 0u;
    unsigned dropped = 0u;
    unsigned accepted_digest = 0u;

    tg14_init();

    (void)tg14_enqueue(17u, &slot0, &gen0);
    (void)tg14_enqueue(29u, &slot1, &gen1);

    if (tg14_accept(slot0, gen0, &value)) {
        accepted += 1u;
        accepted_digest = (accepted_digest * 131u) ^ value;
    } else {
        dropped += 1u;
    }

    tg14_force_reuse(slot1, 77u);

    if (tg14_accept(slot1, gen1, &value)) {
        accepted += 1u;
        accepted_digest = (accepted_digest * 131u) ^ value;
    } else {
        dropped += 1u;
    }

    if (tg14_accept(slot1, gen1 + 1u, &value)) {
        accepted += 1u;
        accepted_digest = (accepted_digest * 131u) ^ value;
    } else {
        dropped += 1u;
    }

    printf("%u %u %u %u\n", accepted, dropped, accepted_digest, tg14_state_digest());
    return 0;
}
