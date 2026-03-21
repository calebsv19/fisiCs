int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    int x = 1;
    int *p = &x;
    p &= 1;
    return x;
}
