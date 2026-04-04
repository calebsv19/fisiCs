#include <stdio.h>

int helper_internal_bump(int delta);
int helper_internal_get(void);

static int main_internal = 5;

static int main_internal_bump(int delta) {
    main_internal += delta;
    return main_internal;
}

int main(void) {
    int a = main_internal_bump(3);
    int b = helper_internal_bump(20);
    int c = helper_internal_bump(-5);

    if (a != 8 || b != 120 || c != 115 || helper_internal_get() != 115) {
        return 1;
    }
    if (main_internal != 8) {
        return 2;
    }

    printf("linkage_static_internal_isolation_ok main=%d helper=%d\n", main_internal, helper_internal_get());
    return 0;
}
