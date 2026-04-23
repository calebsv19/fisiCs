#include <stdio.h>

void own14_reset(void);
void own14_set(unsigned lane, int enabled);
unsigned own14_release_total(void);
unsigned own14_digest(void);

int main(void) {
    unsigned lanes[] = {0u, 1u, 1u, 0u, 2u, 2u, 2u, 1u, 0u, 0u, 1u, 2u, 1u, 2u};
    int enable[] = {1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0};
    unsigned i;

    own14_reset();
    for (i = 0u; i < sizeof(lanes) / sizeof(lanes[0]); ++i) {
        own14_set(lanes[i], enable[i]);
    }

    printf("%u %u\n", own14_release_total(), own14_digest());
    return 0;
}
