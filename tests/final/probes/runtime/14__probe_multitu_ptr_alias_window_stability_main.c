#include <stddef.h>
#include <stdio.h>

void paws_init(unsigned* buf, size_t n);
unsigned paws_window_step(unsigned* buf, size_t n, size_t start, size_t width, unsigned delta);
unsigned paws_state(void);

int main(void) {
    unsigned lane[18];
    unsigned digest = 53u;
    size_t i;

    paws_init(lane, 18u);
    digest = digest * 181u + paws_window_step(lane, 18u, 2u, 5u, 9u);
    digest = digest * 181u + paws_window_step(lane, 18u, 7u, 4u, 13u);
    digest = digest * 181u + paws_window_step(lane, 18u, 1u, 8u, 3u);

    for (i = 0u; i < 18u; ++i) {
        digest = digest * 191u + lane[i];
    }

    printf("%u %u\n", digest, paws_state());
    return 0;
}
