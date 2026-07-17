extern int printf(const char*, ...);

static int called = 0;

static int record_call(int value) {
    called += value;
    return called;
}

int main(void) {
    int state = 1;
    int selected = (state++ ? (state += 4, 11) : record_call((state += 100, state)));
    int result = (0 ? record_call((state += 200, selected)) : (selected + state));

    printf("%d %d %d\n", state, selected, result);
    return 0;
}
