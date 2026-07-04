extern int printf(const char*, ...);

typedef int (*wave22_cb_t)(int);

int wave22_apply_callbacks(wave22_cb_t callbacks[3], int seed);

static int wave22_add1(int value) {
    return value + 1;
}

static int wave22_add2(int value) {
    return value + 2;
}

static int wave22_add3(int value) {
    return value + 3;
}

int wave22_apply_callbacks(wave22_cb_t *callbacks, int seed) {
    return callbacks[0](seed) + callbacks[1](seed) + callbacks[2](seed);
}

int main(void) {
    wave22_cb_t callbacks[3] = {wave22_add1, wave22_add2, wave22_add3};
    printf("%d\n", wave22_apply_callbacks(callbacks, 5));
    return 0;
}
