int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    int x = 8;
    int y = 2;
    int *p = &y;
    x /= p;
    return x;
}
