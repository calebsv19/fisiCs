extern int printf(const char*, ...);

int main(void) {
    int x = 4;
    int width = (int)sizeof(x += 9);
    printf("%d %d\n", x, width);
    return 0;
}
