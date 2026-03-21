int f(int x) {
    return x;
}

int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    int (*fp)(int) = f;
    return fp(1, 2);
}
