#include <stdio.h>

void qgr_reset(void);
unsigned qgr_push(unsigned value, unsigned* out_generation);
unsigned qgr_reclaim(unsigned keep);
int qgr_collect(unsigned slot, unsigned generation, unsigned* out_value);
unsigned qgr_digest(void);

int main(void) {
    unsigned g0;
    unsigned g1;
    unsigned g2;
    unsigned s0;
    unsigned s1;
    unsigned s2;
    unsigned out = 0u;
    unsigned accepted = 0u;
    unsigned dropped = 0u;
    unsigned reclaimed;
    unsigned digest = 23u;

    qgr_reset();

    s0 = qgr_push(17u, &g0);
    s1 = qgr_push(29u, &g1);
    s2 = qgr_push(41u, &g2);

    reclaimed = qgr_reclaim(2u);
    digest = digest * 151u + reclaimed;

    if (qgr_collect(s0, g0, &out)) {
        accepted += 1u;
        digest = digest * 151u + out;
    } else {
        dropped += 1u;
    }
    if (qgr_collect(s1, g1, &out)) {
        accepted += 1u;
        digest = digest * 151u + out;
    } else {
        dropped += 1u;
    }
    if (qgr_collect(s2, g2, &out)) {
        accepted += 1u;
        digest = digest * 151u + out;
    } else {
        dropped += 1u;
    }

    (void)qgr_push(73u, &g2);
    (void)qgr_push(89u, &g1);
    reclaimed = qgr_reclaim(3u);
    digest = digest * 151u + reclaimed;

    printf("%u %u %u %u\n", accepted, dropped, digest, qgr_digest());
    return 0;
}
