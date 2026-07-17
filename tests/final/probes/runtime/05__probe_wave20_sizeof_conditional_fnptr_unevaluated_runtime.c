extern int printf(const char*, ...);

struct Node {
    int value;
    int (*apply)(int*, int);
};

static int bump(int *slot, int amount) {
    *slot += amount;
    return *slot;
}

int main(void) {
    int calls = 0;
    int value = 3;
    struct Node node = {11, bump};

    int size = (int)sizeof((calls ? (node.apply(&value, 100), node.value)
                                  : (node.value += 7, node.apply(&calls, 5))));
    int live = calls ? (node.apply(&value, 40), value)
                     : (calls += 2, value + node.value);

    printf("%d %d %d %d %d\n", calls, value, node.value, size, live);
    return 0;
}
