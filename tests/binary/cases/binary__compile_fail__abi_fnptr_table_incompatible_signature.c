static int wrong(double x, double y) {
    return (int)(x + y);
}

int main(void) {
    int (*table[1])(int, int) = {wrong};
    return table[0](2, 3);
}
