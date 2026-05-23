#include <stdio.h>
#include <stddef.h>

void ssarm_init(unsigned* buf, size_t n, unsigned salt);
unsigned ssarm_step(unsigned* buf, size_t n, size_t start, size_t width, unsigned delta);
unsigned ssarm_state(void);

int main(void) {
    unsigned lane[20];
    unsigned h = 2166136261u;

    ssarm_init(lane, 20u, 7u);
    h = h * 16777619u ^ ssarm_step(lane, 20u, 2u, 6u, 5u);
    h = h * 16777619u ^ ssarm_step(lane, 20u, 9u, 5u, 11u);
    h = h * 16777619u ^ ssarm_step(lane, 20u, 1u, 9u, 3u);
    h = h * 16777619u ^ ssarm_step(lane, 20u, 4u, 7u, 13u);

    printf("%u %u\n", h, ssarm_state());
    return 0;
}
