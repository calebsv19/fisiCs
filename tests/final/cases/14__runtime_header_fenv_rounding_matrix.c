#include <fenv.h>
#include <stdio.h>

int main(void) {
    int original = fegetround();

    if (original == -1) {
        return 1;
    }
    if (fesetround(FE_DOWNWARD) != 0) {
        return 2;
    }
    if (fegetround() != FE_DOWNWARD) {
        return 3;
    }
    if (fesetround(original) != 0) {
        return 4;
    }

    printf(
        "changed=%d restored=%d\n",
        1,
        fegetround() == original ? 1 : 0);
    return 0;
}
