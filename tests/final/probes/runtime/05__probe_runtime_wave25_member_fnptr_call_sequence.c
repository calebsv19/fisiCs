extern int printf(const char*, ...);

struct Node {
    int value;
    int (*op)(int*, int);
};

static int add_path(int *trace, int value) {
    *trace += value;
    return *trace + 1;
}

static int bias_path(int *trace, int value) {
    *trace += value + 2;
    return *trace - value;
}

int main(void) {
    struct Node left = {3, add_path};
    struct Node right = {5, bias_path};
    int trace = 2;
    int choose = 0;

    struct Node *selected = (trace += 4, choose ? &left : &right);
    int arg = (trace += selected->value, selected->value + trace);
    int result = (choose ? left.op : selected->op)(&trace, arg);
    int second = (choose || (trace += 7)) ? selected->value : 0;

    printf("%d %d %d %d\n", trace, arg, result, second);
    return 0;
}
