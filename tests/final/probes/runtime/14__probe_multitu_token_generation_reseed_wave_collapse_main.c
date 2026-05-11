#include <stdio.h>

void tg14_init(void);
int tg14_enqueue(unsigned value, unsigned* out_slot, unsigned* out_generation);
int tg14_accept(unsigned slot, unsigned generation, unsigned* out_value);
void tg14_force_reuse(unsigned slot, unsigned value);
unsigned tg14_state_digest(void);

int main(void) {
    unsigned s[4] = {0u, 0u, 0u, 0u};
    unsigned g[4] = {0u, 0u, 0u, 0u};
    unsigned value = 0u;
    unsigned accepted = 0u;
    unsigned dropped = 0u;
    unsigned digest = 0u;

    tg14_init();

    (void)tg14_enqueue(13u, &s[0], &g[0]);
    (void)tg14_enqueue(27u, &s[1], &g[1]);
    (void)tg14_enqueue(39u, &s[2], &g[2]);
    (void)tg14_enqueue(51u, &s[3], &g[3]);

    tg14_force_reuse(s[2], 65u);
    tg14_force_reuse(s[2], 77u);
    tg14_force_reuse(s[3], 89u);

    if (tg14_accept(s[2], g[2], &value)) {
        accepted += 1u;
        digest = (digest * 173u) ^ value;
    } else {
        dropped += 1u;
    }

    if (tg14_accept(s[2], g[2] + 1u, &value)) {
        accepted += 1u;
        digest = (digest * 173u) ^ value;
    } else {
        dropped += 1u;
    }

    if (tg14_accept(s[2], g[2] + 2u, &value)) {
        accepted += 1u;
        digest = (digest * 173u) ^ value;
    } else {
        dropped += 1u;
    }

    if (tg14_accept(s[3], g[3], &value)) {
        accepted += 1u;
        digest = (digest * 173u) ^ value;
    } else {
        dropped += 1u;
    }

    if (tg14_accept(s[3], g[3] + 1u, &value)) {
        accepted += 1u;
        digest = (digest * 173u) ^ value;
    } else {
        dropped += 1u;
    }

    if (tg14_accept(s[1], g[1], &value)) {
        accepted += 1u;
        digest = (digest * 173u) ^ value;
    } else {
        dropped += 1u;
    }

    printf("%u %u %u %u\n", accepted, dropped, digest, tg14_state_digest());
    return 0;
}
