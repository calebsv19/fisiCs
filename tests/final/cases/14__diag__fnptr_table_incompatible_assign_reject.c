typedef int (*op_fn)(int, int);

static int add(int a, int b) { return a + b; }

int main(void) {
    op_fn table[1] = {add};
    return table[0](1);
}
