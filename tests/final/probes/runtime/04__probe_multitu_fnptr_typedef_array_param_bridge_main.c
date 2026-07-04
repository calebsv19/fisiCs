extern int printf(const char*, ...);

typedef int (*wave22_cb_t)(int);

extern int wave22_multitu_apply_callbacks(wave22_cb_t callbacks[2], int seed);

static int wave22_add4(int value) {
    return value + 4;
}

static int wave22_add7(int value) {
    return value + 7;
}

int main(void) {
    wave22_cb_t callbacks[2] = {wave22_add4, wave22_add7};
    printf("%d\n", wave22_multitu_apply_callbacks(callbacks, 8));
    return 0;
}
