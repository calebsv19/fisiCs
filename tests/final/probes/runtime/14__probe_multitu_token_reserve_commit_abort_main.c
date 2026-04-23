#include <stdio.h>

void rs14_init(void);
int rs14_reserve(unsigned* out_slot, unsigned* out_generation);
int rs14_commit(unsigned slot, unsigned generation, unsigned value);
int rs14_abort(unsigned slot, unsigned generation);
int rs14_read(unsigned slot, unsigned generation, unsigned* out_value);
unsigned rs14_digest(void);

int main(void) {
    unsigned s0 = 0u;
    unsigned g0 = 0u;
    unsigned s1 = 0u;
    unsigned g1 = 0u;
    unsigned s2 = 0u;
    unsigned g2 = 0u;
    unsigned s3 = 0u;
    unsigned g3 = 0u;
    unsigned s4 = 0u;
    unsigned g4 = 0u;
    unsigned value = 0u;
    unsigned ok = 0u;
    unsigned reject = 0u;
    unsigned digest = 0u;

    rs14_init();

    (void)rs14_reserve(&s0, &g0);
    rs14_commit(s0, g0, 17u) ? (ok += 1u) : (reject += 1u);

    (void)rs14_reserve(&s1, &g1);
    rs14_abort(s1, g1) ? (ok += 1u) : (reject += 1u);

    if (rs14_read(s1, g1, &value)) {
        ok += 1u;
        digest = (digest * 149u) ^ value;
    } else {
        reject += 1u;
    }

    (void)rs14_reserve(&s2, &g2);
    (void)rs14_abort(s2, g2);
    (void)rs14_reserve(&s3, &g3);
    (void)rs14_abort(s3, g3);

    (void)rs14_reserve(&s4, &g4);
    rs14_commit(s4, g4, 99u) ? (ok += 1u) : (reject += 1u);

    if (rs14_read(s0, g0, &value)) {
        ok += 1u;
        digest = (digest * 149u) ^ value;
    } else {
        reject += 1u;
    }

    if (rs14_read(s0, g0 + 1u, &value)) {
        ok += 1u;
        digest = (digest * 149u) ^ value;
    } else {
        reject += 1u;
    }

    if (rs14_read(s4, g4, &value)) {
        ok += 1u;
        digest = (digest * 149u) ^ value;
    } else {
        reject += 1u;
    }

    printf("%u %u %u %u\n", ok, reject, digest, rs14_digest());
    return 0;
}
