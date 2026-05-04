extern int printf(const char*, ...);

typedef int (*wave13_cb_t)(int);
extern int wave14_multitu_fnptr_bridge(wave13_cb_t cb, int seed);

static int wave14_inc3(int value) {
    return value + 3;
}

int main(void) {
    printf("%d\n", wave14_multitu_fnptr_bridge(wave14_inc3, 9));
    return 0;
}
