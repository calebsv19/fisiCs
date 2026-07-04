extern int printf(const char*, ...);

int main(void) {
    int *values = (int[]){3, 5, 8};
    unsigned total = (unsigned)values[0] + (unsigned)values[2];

    printf("%d %u\n", values[1], total);
    return 0;
}
