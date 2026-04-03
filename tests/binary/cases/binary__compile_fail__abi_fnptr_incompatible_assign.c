static int unary(int x) {
    return x + 1;
}

int main(void) {
    int (*op)(int, int);
    op = unary;
    return 0;
}
