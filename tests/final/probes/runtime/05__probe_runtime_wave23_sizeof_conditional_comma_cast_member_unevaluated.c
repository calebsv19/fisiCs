extern int printf(const char*, ...);

struct Handler {
    int bias;
    int (*fn)(int*, int);
};

static int add_trace(int *trace, int amount) {
    *trace += amount;
    return *trace;
}

int main(void) {
    struct Handler a = {4, add_trace};
    struct Handler b = {9, add_trace};
    struct Handler *pa = &a;
    struct Handler *pb = &b;
    int choose = 0;
    int trace = 3;

    int width = (int)sizeof(((trace += 100), (choose ? pa : pb)->fn)(&trace, (int)((choose ? pa : pb)->bias)));
    int selected_bias = *((int *)(void *)((trace += 5), choose ? &pa->bias : &pb->bias));
    int value = (choose ? pa : pb)->fn(&trace, width + selected_bias);

    printf("%d %d %d %d\n", trace, width, selected_bias, value);
    return 0;
}
