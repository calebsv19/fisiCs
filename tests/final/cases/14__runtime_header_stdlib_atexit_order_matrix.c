#include <stdio.h>
#include <stdlib.h>

static int state = 3;

static void first_handler(void) {
    state = state * 10 + 1;
    printf("stdlib-atexit-first state=%d\n", state);
}

static void second_handler(void) {
    state = state * 10 + 2;
    printf("stdlib-atexit-second state=%d\n", state);
}

int main(void) {
    int r1 = atexit(first_handler);
    int r2 = atexit(second_handler);
    printf("stdlib-atexit-main state=%d registrations=%d/%d\n", state, r1, r2);
    return r1 == 0 && r2 == 0 ? 0 : 1;
}
