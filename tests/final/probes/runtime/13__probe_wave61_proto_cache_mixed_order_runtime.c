#include <stdio.h>

typedef int (*wave61_unspecified_fn)();
typedef int (*wave61_void_fn)(void);

int wave61_empty_first();
int wave61_empty_first(void);

int wave61_void_first(void);
int wave61_void_first();

int wave61_empty_first(void) {
    return 17;
}

int wave61_void_first(void) {
    return 29;
}

static int wave61_call_unspecified(wave61_unspecified_fn fn) {
    return fn();
}

static int wave61_call_void(wave61_void_fn fn) {
    return fn();
}

int main(void) {
    wave61_unspecified_fn unspecified_empty = wave61_empty_first;
    wave61_unspecified_fn unspecified_void = wave61_void_first;
    wave61_void_fn void_empty = wave61_empty_first;
    wave61_void_fn void_void = wave61_void_first;
    int direct = wave61_empty_first() + wave61_void_first();
    int indirect = wave61_call_unspecified(unspecified_empty)
        + wave61_call_unspecified(unspecified_void)
        + wave61_call_void(void_empty)
        + wave61_call_void(void_void);

    printf("%d %d %d\n", direct, indirect, direct + indirect);
    return 0;
}
