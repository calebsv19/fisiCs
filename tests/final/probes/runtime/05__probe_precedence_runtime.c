int main(void) {
    int a = 2;
    int b = 3;
    int c = 4;
    int d = 5;
    int v = a + b * c - d / 2 + ((a < b && c > d) ? 7 : 11);
    return v;
}
