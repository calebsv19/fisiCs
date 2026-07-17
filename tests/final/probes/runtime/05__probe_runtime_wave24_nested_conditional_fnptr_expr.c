extern int printf(const char*, ...);

struct Handler {
    int bias;
    int (*fn)(int, int);
};

static int add_bias(int value, int bias) {
    return value + bias;
}

static int scale_bias(int value, int bias) {
    return value * bias;
}

static int sub_bias(int value, int bias) {
    return value - bias;
}

int main(void) {
    struct Handler handlers[3] = {
        {3, add_bias},
        {4, scale_bias},
        {7, sub_bias},
    };
    int trace = 1;
    int outer = 0;
    int inner = 1;

    struct Handler *picked = (trace += 2, outer ? &handlers[0] : (inner ? &handlers[1] : &handlers[2]));
    int first = (outer ? handlers[0].fn : (inner ? picked->fn : handlers[2].fn))((trace += 5), picked->bias);

    inner = 0;
    trace += 11;
    struct Handler *picked2 = outer ? &handlers[0] : (inner ? &handlers[1] : &handlers[2]);
    int second = (outer ? handlers[0].fn : (inner ? handlers[1].fn : picked2->fn))((trace += 13), picked2->bias);

    printf("%d %d %d %d\n", trace, first, second, picked2->bias);
    return 0;
}
