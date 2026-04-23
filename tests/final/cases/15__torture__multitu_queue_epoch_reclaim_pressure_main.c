#include <stdio.h>

int qe62_submit(unsigned value, int should_fail);
unsigned qe62_reclaim(unsigned min_epoch);
unsigned qe62_epoch(void);
unsigned qe62_digest(void);
int qe62_should_fail(unsigned step);

int main(void) {
    unsigned i;
    unsigned ok = 0u;
    unsigned fail = 0u;
    unsigned reclaimed = 0u;
    unsigned mirror = 2166136261u;

    for (i = 0u; i < 128u; ++i) {
        int should_fail = qe62_should_fail(i);
        if (qe62_submit(i * 41u + 17u, should_fail)) {
            ok += 1u;
            mirror = (mirror ^ (i * 131u + 7u)) * 16777619u;
        } else {
            fail += 1u;
            mirror = (mirror ^ (i * 17u + 3u)) * 16777619u;
        }

        if ((i % 9u) == 5u) {
            unsigned epoch = qe62_epoch();
            unsigned cut = (epoch > 2u) ? (epoch - 2u) : 0u;
            reclaimed += qe62_reclaim(cut);
        }
    }

    printf("%u %u %u %u\n", ok, fail, reclaimed, qe62_digest() ^ mirror);
    return 0;
}
