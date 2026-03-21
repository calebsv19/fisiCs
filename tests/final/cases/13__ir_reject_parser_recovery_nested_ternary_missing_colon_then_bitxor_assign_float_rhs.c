int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;
    int broken = a ? (b ? 4 5) : c;
    int x = 1;
    float y = 2.0f;
    x ^= y;
    return x;
}
