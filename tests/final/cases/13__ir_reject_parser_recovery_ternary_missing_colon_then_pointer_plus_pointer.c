int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    int a = 4;
    int b = 5;
    int* p = &a;
    int* q = &b;
    return (int)(p + q);
}
