int add1(int x) {
    return x + 1;
}

int main(void) {
    int (*fp)(int) = add1;
    int (**pp)(int) = &fp;
    int (** const *src)(int) = &pp;
    int (***dst)(int);
    dst = src;
    return 0;
}
