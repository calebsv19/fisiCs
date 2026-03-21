int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    int x = 8;
    int *p = &x;
    p %= 3;
    return x;
}
