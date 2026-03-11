int main(void) {
    int a = 1;
    int b = 2;
    int c = 0;
    (c ? a : b) = 3;
    return a + b;
}
