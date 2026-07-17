extern int printf(const char*, ...);

struct Node {
    int bias;
    int (*apply)(int);
};

static int plus_three(int value) {
    return value + 3;
}

static int times_four(int value) {
    return value * 4;
}

int main(void) {
    struct Node table[2] = {
        {5, plus_three},
        {2, times_four},
    };
    int state = 0;
    int use_right = 1;

    struct Node *picked = (use_right ? (state += table[0].bias, &table[1])
                                    : (state += 100, &table[0]));
    int value = picked->apply((state += picked->bias, state));

    printf("%d %d\n", state, value);
    return 0;
}
