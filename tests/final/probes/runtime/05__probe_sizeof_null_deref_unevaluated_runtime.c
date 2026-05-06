extern int printf(const char*, ...);

int main(void) {
    int *ptr = 0;
    printf("%d\n", (int)sizeof(*ptr));
    return 0;
}
