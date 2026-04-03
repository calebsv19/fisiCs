#include <stdio.h>

int main(void) {
    FILE* out = fopen("numbers.txt", "w");
    if (!out) return 2;
    fputs("1\n2\n", out);
    fclose(out);

    out = fopen("numbers.txt", "a");
    if (!out) return 3;
    fputs("3\n", out);
    fclose(out);

    FILE* in = fopen("numbers.txt", "r");
    if (!in) return 4;
    int value = 0;
    int total = 0;
    while (fscanf(in, "%d", &value) == 1) {
        total += value;
    }
    fclose(in);

    printf("%d\n", total);
    return 0;
}

