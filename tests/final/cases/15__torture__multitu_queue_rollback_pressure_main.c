#include <stdio.h>

int qb15_submit(unsigned value, int should_fail);
unsigned qb15_digest(void);
unsigned qb15_state_count(void);
int qb15_should_fail(unsigned step);

int main(void) {
    unsigned i;
    unsigned ok_count = 0u;
    unsigned fail_count = 0u;
    unsigned mirror = 2166136261u;

    for (i = 0u; i < 96u; ++i) {
        int should_fail = qb15_should_fail(i);
        int ok = qb15_submit(i * 37u + 11u, should_fail);

        if (ok) {
            ok_count += 1u;
            mirror = (mirror ^ ((i + 1u) * 131u)) * 16777619u;
        } else {
            fail_count += 1u;
            mirror = (mirror ^ ((i + 1u) * 17u)) * 16777619u;
        }
    }

    printf("%u %u %u %u\n", ok_count, fail_count, mirror, qb15_digest() ^ qb15_state_count());
    return 0;
}
