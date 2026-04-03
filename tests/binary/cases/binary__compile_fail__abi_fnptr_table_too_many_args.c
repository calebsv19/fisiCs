typedef int (*op_fn)(int, int);

static int add2(int a, int b) {
    return a + b;
}

int main(void) {
    op_fn table[1] = {add2};
    return table[0](1, 2, 3);
}
