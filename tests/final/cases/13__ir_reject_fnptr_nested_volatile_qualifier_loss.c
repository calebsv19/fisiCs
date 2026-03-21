int add1(int x) {
    return x + 1;
}

int main(void) {
    int (*fp)(int) = add1;
    int (* volatile *src)(int) = &fp;
    int (**dst)(int);
    dst = src;
    return 0;
}
