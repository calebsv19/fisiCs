#include <stdbool.h>

struct Wave21BoolConversionSurface {
    bool slots[4];
};

int wave21_stdbool_conversion_surface(struct Wave21BoolConversionSurface *state, int seed) {
    int i;
    int total = 0;

    for (i = 0; i < 4; ++i) {
        state->slots[i] = (bool)(seed & (1 << i));
        total += state->slots[i] ? (i + 1) : 0;
    }
    return total;
}
