extern int printf(const char*, ...);

struct Method {
    int bias;
    int (*apply)(int, int);
};

struct Carrier {
    int tag;
    struct Method method;
};

static int add_bias(int value, int bias) {
    return value + bias;
}

static int scale_bias(int value, int bias) {
    return value * bias;
}

int main(void) {
    struct Carrier table[3] = {
        {1, {2, add_bias}},
        {2, {3, scale_bias}},
        {3, {5, add_bias}},
    };
    int state = 0;
    int pick = 1;

    struct Carrier *selected =
        pick ? (state += table[0].tag, &table[1])
             : (state += 100, &table[2]);
    int arg = (state += selected->method.bias, state + selected->tag);
    int result = selected->method.apply(arg, selected->method.bias);

    printf("%d %d %d\n", state, arg, result);
    return 0;
}
