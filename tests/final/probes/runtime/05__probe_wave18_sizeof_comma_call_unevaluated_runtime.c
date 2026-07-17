extern int printf(const char*, ...);

static int calls = 0;

static int bump_and_return(int *slot, int value) {
    calls += 1;
    *slot += value;
    return *slot;
}

int main(void) {
    int state = 7;
    unsigned size = (unsigned)sizeof(bump_and_return(&state, (state += 100, 3)));
    int after = (state += 5, state);

    printf("%d %d %u %d\n", calls, state, size, after);
    return 0;
}
