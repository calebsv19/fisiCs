extern int printf(const char*, ...);

static int add_two(int value) {
    return value + 2;
}

static int add_five(int value) {
    return value + 5;
}

static int add_nine(int value) {
    return value + 9;
}

int main(void) {
    int state = 1;
    int value = (state++ ? (state++ ? add_two : add_five) : add_nine)(10);

    printf("%d %d\n", state, value);
    return 0;
}
