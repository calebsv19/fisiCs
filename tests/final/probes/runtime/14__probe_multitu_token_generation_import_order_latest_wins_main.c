#include <stdio.h>

void tg14_init(void);
int tg14_enqueue(unsigned value, unsigned* out_slot, unsigned* out_generation);
int tg14_accept(unsigned slot, unsigned generation, unsigned* out_value);
void tg14_force_reuse(unsigned slot, unsigned value);
unsigned tg14_state_digest(void);

int main(void) {
    unsigned s0 = 0u;
    unsigned g0 = 0u;
    unsigned s1 = 0u;
    unsigned g1 = 0u;
    unsigned s2 = 0u;
    unsigned g2 = 0u;
    unsigned value = 0u;
    unsigned accepted = 0u;
    unsigned dropped = 0u;
    unsigned digest = 0u;

    tg14_init();

    (void)tg14_enqueue(13u, &s0, &g0);
    (void)tg14_enqueue(29u, &s1, &g1);
    tg14_force_reuse(s0, 61u);
    (void)tg14_enqueue(47u, &s2, &g2);
    tg14_force_reuse(s1, 83u);

    if (tg14_accept(s0, g0, &value)) {
        accepted += 1u;
        digest = (digest * 131u) ^ value;
    } else {
        dropped += 1u;
    }
    if (tg14_accept(s1, g1, &value)) {
        accepted += 1u;
        digest = (digest * 131u) ^ value;
    } else {
        dropped += 1u;
    }
    if (tg14_accept(s2, g2, &value)) {
        accepted += 1u;
        digest = (digest * 131u) ^ value;
    } else {
        dropped += 1u;
    }
    if (tg14_accept(s0, g0 + 1u, &value)) {
        accepted += 1u;
        digest = (digest * 131u) ^ value;
    } else {
        dropped += 1u;
    }

    tg14_force_reuse(s2, 101u);

    if (tg14_accept(s2, g2, &value)) {
        accepted += 1u;
        digest = (digest * 131u) ^ value;
    } else {
        dropped += 1u;
    }
    if (tg14_accept(s2, g2 + 1u, &value)) {
        accepted += 1u;
        digest = (digest * 131u) ^ value;
    } else {
        dropped += 1u;
    }
    if (tg14_accept(s1, g1 + 1u, &value)) {
        accepted += 1u;
        digest = (digest * 131u) ^ value;
    } else {
        dropped += 1u;
    }

    printf("%u %u %u %u\n", accepted, dropped, digest, tg14_state_digest());
    return 0;
}
