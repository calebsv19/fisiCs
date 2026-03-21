int add2(int a, int b) {
    return a + b;
}

int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    int (*fp)(int, int) = add2;
    return fp(1);
}
