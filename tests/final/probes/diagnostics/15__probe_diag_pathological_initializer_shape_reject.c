#include <stddef.h>

struct Inner {
    int x;
    int y;
};

struct Wrap {
    int values[2];
    struct Inner in;
};

struct Wrap g_cfg = {
    .values = { [0] = 7, [1] = 11 },
    .in = { .x = 3, .y = }
};

int main(void) {
    return g_cfg.values[0];
}
