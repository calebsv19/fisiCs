volatile int g = 0;

int main(void) {
    volatile int l = 1;
    int x = l;
    g = x;
    l = 2;
    return g + x + l;
}
