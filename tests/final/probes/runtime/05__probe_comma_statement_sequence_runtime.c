extern int printf(const char*, ...);

int main(void) {
    int x = 1;
    int y = 10;
    int value;

    value = (x += 2, y = x + y, y - x);
    printf("%d %d %d\n", x, y, value);
    return 0;
}
