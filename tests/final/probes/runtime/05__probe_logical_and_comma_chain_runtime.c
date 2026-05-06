extern int printf(const char*, ...);

int main(void) {
    int x = 1;
    int y = 0;
    int value = (x += 1) && (y = x + 4, y - 1);
    printf("%d %d %d\n", x, y, value);
    return 0;
}
