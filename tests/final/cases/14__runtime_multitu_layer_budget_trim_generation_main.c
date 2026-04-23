#include <stdio.h>

void lb14_reset(void);
void lb14_toggle(unsigned lane, int enabled);
void lb14_touch(unsigned lane, unsigned tiles);
unsigned lb14_token(unsigned lane);
int lb14_collect(unsigned lane, unsigned generation, unsigned* out_value);
unsigned lb14_digest(void);

int main(void) {
    unsigned stale_token;
    unsigned new_token;
    unsigned collected = 0u;
    unsigned ok = 0u;
    unsigned drop = 0u;

    lb14_reset();

    lb14_toggle(1u, 1);
    lb14_touch(1u, 6u);
    stale_token = lb14_token(1u);

    lb14_toggle(1u, 0);
    lb14_toggle(1u, 1);
    lb14_touch(1u, 4u);
    new_token = lb14_token(1u);

    if (lb14_collect(1u, stale_token, &collected)) {
        ok += 1u;
    } else {
        drop += 1u;
    }

    if (lb14_collect(1u, new_token, &collected)) {
        ok += 1u;
    } else {
        drop += 1u;
    }

    lb14_toggle(0u, 1);
    lb14_touch(0u, 9u);
    lb14_toggle(0u, 0);

    lb14_toggle(2u, 1);
    lb14_touch(2u, 7u);
    lb14_toggle(2u, 0);

    printf("%u %u %u %u\n", ok, drop, collected, lb14_digest());
    return 0;
}
