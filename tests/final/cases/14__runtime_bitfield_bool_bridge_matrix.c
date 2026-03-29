#include <stdbool.h>
#include <stdio.h>

struct Flags {
    unsigned int ready : 1;
    unsigned int error : 1;
    unsigned int mode : 2;
};

static unsigned step(struct Flags *f, unsigned in) {
    bool gate = (bool)(in & 1u);
    f->ready = (unsigned)gate;
    f->error = (unsigned)!gate;
    f->mode = (f->mode + (in & 3u)) & 3u;

    bool path = (bool)f->ready && !(bool)f->error;
    return (unsigned)path + f->mode * 10u + f->ready * 100u + f->error * 1000u;
}

int main(void) {
    struct Flags f = {1u, 0u, 2u};
    unsigned checksum = 0u;

    for (unsigned i = 0; i < 6u; ++i) {
        checksum = checksum * 17u + step(&f, i + 2u);
    }

    printf("%u %u %u %u\n", f.ready, f.error, f.mode, checksum);
    return 0;
}
