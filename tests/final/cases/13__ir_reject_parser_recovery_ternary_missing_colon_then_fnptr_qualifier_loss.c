int add1(int x) {
    return x + 1;
}

int main(void) {
    int keep = 1;
    int broken = keep ? 2 3;
    int (*fp)(int) = add1;
    int (* const *src)(int) = &fp;
    int (**dst)(int);
    dst = src;
    return 0;
}
