extern int printf(const char*, ...);

struct Method {
    int factor;
    int (*apply)(int, int);
};

static int add_method(int left, int right) {
    return left + right;
}

static int scale_method(int left, int right) {
    return left * right;
}

static int diff_method(int left, int right) {
    return left - right;
}

int main(void) {
    struct Method methods[3] = {
        {2, add_method},
        {4, scale_method},
        {6, diff_method},
    };
    int calls = 0;
    int gate = 1;
    int left = 2;

    struct Method selected = gate ? (calls += 1, methods[1])
                                  : (calls += 10, methods[2]);
    int arg_left = (left += 3);
    int arg_right = (calls += 2, selected.factor);
    int first = selected.apply(arg_left, arg_right);
    int second = (gate = 0, gate) ? methods[0].apply(first, 1)
                                  : (calls += 5, methods[2].apply(first, calls));

    printf("%d %d %d %d\n", calls, left, first, second);
    return 0;
}
