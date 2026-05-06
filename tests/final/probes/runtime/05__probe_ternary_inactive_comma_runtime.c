extern int printf(const char*, ...);

int main(void) {
    int x = 0;
    int y = 1 ? (x += 2, x + 5) : (x += 100, x);
    printf("%d %d\n", x, y);
    return 0;
}
