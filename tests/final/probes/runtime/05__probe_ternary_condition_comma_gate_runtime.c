extern int printf(const char*, ...);

int main(void) {
    int x = 1;
    int y = 0;
    int z = 0;
    int value = (x += 2, x - 1) ? (y += 5) : (z += 9);
    printf("%d %d %d %d\n", x, y, z, value);
    return 0;
}
