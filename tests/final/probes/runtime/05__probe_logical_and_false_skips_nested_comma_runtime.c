extern int printf(const char*, ...);

int main(void) {
    int x = 0;
    int y = 0;
    int z = 0;
    int value = x && (y += 2, z += 5, z + 1);
    printf("%d %d %d %d\n", x, y, z, value);
    return 0;
}
