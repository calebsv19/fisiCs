int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;
    int broken = a ? (b ? 4 5) : c;
    float f = 1.0f;
    int y = (int)(f & 1);
    return y;
}
