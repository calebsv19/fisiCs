int main(void) {
    int a = 10;
    int b = 3;
    int c = (a + b) - (a / b) + (a % b) * 2;
    return c == 12 ? 0 : 1;
}
