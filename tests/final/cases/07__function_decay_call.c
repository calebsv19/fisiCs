int add(int a, int b) {
    return a + b;
}

int apply(int (*fn)(int, int), int x, int y) {
    return fn(x, y);
}

int main(void) {
    return apply(add, 2, 5);
}
