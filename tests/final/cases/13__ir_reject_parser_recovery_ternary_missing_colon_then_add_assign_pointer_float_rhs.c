int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    int x = 8;
    int *p = &x;
    float y = 1.0f;
    p += y;
    return x;
}
