#include <stdio.h>

typedef int (*wave61_control_void_fn)(void);
typedef int (*wave61_control_unspecified_fn)();

int wave61_control_void_first(void);
int wave61_control_void_first();

int wave61_control_empty_first();
int wave61_control_empty_first(void);

int wave61_control_void_first(void) {
    return 29;
}

int wave61_control_empty_first(void) {
    return 17;
}

static int wave61_control_call_void(wave61_control_void_fn fn) {
    return fn();
}

static int wave61_control_call_unspecified(wave61_control_unspecified_fn fn) {
    return fn();
}

int main(void) {
    wave61_control_void_fn void_void = wave61_control_void_first;
    wave61_control_void_fn void_empty = wave61_control_empty_first;
    wave61_control_unspecified_fn unspecified_void = wave61_control_void_first;
    wave61_control_unspecified_fn unspecified_empty = wave61_control_empty_first;
    int direct = wave61_control_void_first() + wave61_control_empty_first();
    int indirect = wave61_control_call_void(void_void)
        + wave61_control_call_void(void_empty)
        + wave61_control_call_unspecified(unspecified_void)
        + wave61_control_call_unspecified(unspecified_empty);

    printf("%d %d %d\n", direct, indirect, direct + indirect);
    return 0;
}
