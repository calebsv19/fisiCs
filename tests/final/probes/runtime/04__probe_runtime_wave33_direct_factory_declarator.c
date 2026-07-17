extern int printf(const char*, ...);

static int wave33_add_three(int value) {
    return value + 3;
}

static int wave33_double(int value) {
    return value * 2;
}

static int (*wave33_choose_transform(int which))(int) {
    return which ? wave33_double : wave33_add_three;
}

int main(void) {
    int (*(*factory)(int))(int) = wave33_choose_transform;
    int (*first)(int) = factory(0);
    int (*second)(int) = factory(1);
    printf("%d\n", first(8) + second(8));
    return 0;
}
