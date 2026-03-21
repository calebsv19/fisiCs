int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;
    int broken = a ? (b ? 4 5) : c;
    float x = 3.0f;
    return (int)(x % 2.0f);
}
