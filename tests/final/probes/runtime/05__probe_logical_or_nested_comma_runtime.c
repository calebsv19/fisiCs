extern int printf(const char*, ...);

int main(void) {
    int x = 0;
    int y = 0;
    int z = 0;
    int value = (x += 1) || (y += 4, z += 9, z);
    printf("%d %d %d %d\n", x, y, z, value);
    return 0;
}
