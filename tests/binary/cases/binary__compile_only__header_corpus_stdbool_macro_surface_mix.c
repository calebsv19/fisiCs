#include <stdbool.h>

enum {
    wave21_bool_defined = 1 / (__bool_true_false_are_defined == 1),
    wave21_true_value = 1 / (true == 1),
    wave21_false_value = 1 / (false == 0)
};

struct Wave21BoolMacroSurface {
    bool enabled;
    bool visible;
};

int wave21_stdbool_macro_surface(struct Wave21BoolMacroSurface *state, int mask) {
    state->enabled = (mask & 1) ? true : false;
    state->visible = (mask & 2) ? true : false;
    return state->enabled && !state->visible;
}
