int add1(int x) {
    return x + 1;
}

int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;
    int broken = a ? (b ? 4 5) : c;
    int (*fp)(int) = add1;
    int (**pp)(int) = &fp;
    int (** const *src)(int) = &pp;
    int (***dst)(int);
    dst = src;
    return 0;
}
