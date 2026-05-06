extern int printf(const char*, ...);

int main(void) {
    int x = 0;
    int y = 0;
    int z = 0;
    int value = 0 ? (x += 10) : ((y += 3) ? (z += 4) : (z += 20));
    printf("%d %d %d %d\n", x, y, z, value);
    return 0;
}
