extern int printf(const char*, ...);

struct Node {
    int value;
    int (*apply)(int*, int);
};

static int bump(int *trace, int amount) {
    *trace += amount;
    return *trace;
}

int main(void) {
    struct Node left = {3, bump};
    struct Node right = {7, bump};
    int pick = 0;
    int trace = 1;

    int width = (int)sizeof(((trace += 40), ((pick ? &left : &right)->apply)(&trace, 5)));
    int value = ((trace += 2), pick ? &left : &right)->apply(&trace, width);

    printf("%d %d %d %d\n", trace, width, value, right.value);
    return 0;
}
