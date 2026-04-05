#include <stdio.h>

typedef struct PtrPair {
    int* p;
    const int* cp;
} PtrPair;

static int step_state(void) {
    static int attachment = 7;
    static PtrPair state = {
        .p = &attachment,
        .cp = &attachment,
    };

    attachment += 1;
    if (state.p != &attachment || state.cp != &attachment) {
        return -1000;
    }
    *state.p += 2;
    return attachment;
}

int main(void) {
    int a = step_state();
    int b = step_state();
    printf("%d %d\n", a, b);
    if (a != 10 || b != 13) {
        return 1;
    }
    return 0;
}
