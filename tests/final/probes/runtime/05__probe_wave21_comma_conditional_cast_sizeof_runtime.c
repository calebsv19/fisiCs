extern int printf(const char*, ...);

int main(void) {
    int a = 1;
    int b = 2;
    int c = 3;

    int first = ((a += 2), (b += a), b)
        ? (int)(sizeof(int) + (c += 4))
        : (c += 40);
    int second = ((a -= 3), a)
        ? (b += 80)
        : ((int)sizeof((b += 50, c)) + (b += 5));
    int third = (int)sizeof((a ? (b += 100) : (c += 100))) + (a += 1);

    printf("%d %d %d %d %d %d\n", a, b, c, first, second, third);
    return 0;
}
