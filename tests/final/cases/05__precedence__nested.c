int main(void) {
    int a = 3;
    int b = 4;
    int c = 5;
    int v = ((a + b) * (c - a)) / (b - 2) + (a ? b : c);
    return v == 11 ? 0 : 1;
}
