extern int printf(const char*, ...);

int main(void) {
    int values[4] = {10, 20, 30, 40};
    int trace = 0;
    int flip = 1;

    int *p = (int *)(void *)((trace += 2), (flip ? &values[2] : &values[0]));
    *p += (int)sizeof((trace += 50, &values[1])) + (trace += 3, values[0]);

    flip = 0;
    int *q = (int *)(void *)((trace += 5), (flip ? &values[3] : &values[1]));
    *q += (trace += 7, *p) + (int)sizeof((flip ? values[0] : values[3]));

    printf("%d %d %d %d %d\n", trace, values[0], values[1], values[2], values[3]);
    return 0;
}
