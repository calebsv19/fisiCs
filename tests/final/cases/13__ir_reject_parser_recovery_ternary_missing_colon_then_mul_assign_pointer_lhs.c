int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    int x = 7;
    int *p = &x;
    p *= 2;
    return x;
}
