int f(int x) {
    return x;
}

int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;
    int broken = a ? (b ? 4 5) : c;
    int (*fp)(int) = f;
    return fp(1, 2);
}
