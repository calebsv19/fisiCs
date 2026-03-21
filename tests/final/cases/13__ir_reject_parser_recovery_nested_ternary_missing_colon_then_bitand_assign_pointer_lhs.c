int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;
    int broken = a ? (b ? 4 5) : c;
    int x = 1;
    int *p = &x;
    p &= 1;
    return x;
}
