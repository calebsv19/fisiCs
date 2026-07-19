#include <stdbool.h>

static bool pointer_state(int *value, bool use_null) {
    if (use_null) {
        return (void *)0;
    }
    return value;
}

int main(void) {
    int value = 7;

    if (pointer_state(&value, false) != true) {
        return 1;
    }
    if (pointer_state(&value, true) != false) {
        return 2;
    }
    return 0;
}
