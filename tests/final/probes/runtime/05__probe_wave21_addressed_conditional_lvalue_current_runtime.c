extern int printf(const char*, ...);

int main(void) {
    int arr[4] = {3, 5, 7, 11};
    int idx = 0;
    int trace = 0;

    int *slot = ((idx += 1), idx == 1)
        ? (int *)(void *)&arr[(trace += 2, 2)]
        : (int *)(void *)&arr[0];
    *slot += (int)sizeof((trace += 50, arr[0])) + (trace += 3, arr[idx]);

    int *slot2 = (trace += 7, trace > 10)
        ? (int *)(void *)&arr[3]
        : (int *)(void *)&arr[0];
    *slot2 += (trace += 1, *slot);

    printf("%d %d %d %d %d %d\n", idx, trace, arr[0], arr[1], arr[2], arr[3]);
    return 0;
}
