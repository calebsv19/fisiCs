extern int printf(const char*, ...);

int main(void) {
    int x = 0;
    int y = 1;
    int z = 2;
    int value = (x += 0) || (y += 3, z += y, z - 1);
    printf("%d %d %d %d\n", x, y, z, value);
    return 0;
}
