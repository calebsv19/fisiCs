typedef int (*FP)(int, int);

int add(int a, int b) { return a + b; }

int main(void) {
    FP f = add;
    return f(2, 5);
}
