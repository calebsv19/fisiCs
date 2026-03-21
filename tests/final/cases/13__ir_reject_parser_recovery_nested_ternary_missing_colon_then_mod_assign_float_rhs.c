int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;
    int broken = a ? (b ? 4 5) : c;
    int x = 8;
    float y = 3.0f;
    x %= y;
    return x;
}
