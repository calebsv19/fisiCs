int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;
    int broken = a ? (b ? 4 5) : c;
    float x = 1.0f;
    int y = 2;
    x &= y;
    return 0;
}
