int main(void) {
    int a = 1;
    int b = 2;
    (1 ? a : b) = 3;
    return a + b;
}
