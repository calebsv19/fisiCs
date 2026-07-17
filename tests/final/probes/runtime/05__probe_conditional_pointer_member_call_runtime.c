extern int printf(const char*, ...);

struct Ops {
    int bias;
    int (*apply)(int, int);
};

static int add_bias(int value, int bias) {
    return value + bias;
}

static int mul_bias(int value, int bias) {
    return value * bias;
}

int main(void) {
    struct Ops ops[3] = {
        {3, add_bias},
        {4, mul_bias},
        {5, add_bias},
    };
    int index = 0;
    int choose_left = 0;

    int value = (choose_left ? &ops[index++] : &ops[++index])->apply(
        6,
        choose_left ? ops[0].bias : ops[index].bias
    );

    printf("%d %d\n", index, value);
    return 0;
}
