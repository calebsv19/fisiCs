#include <stdio.h>

int main(void) {
    int count = 0;
    int ch = 0;
    while ((ch = getchar()) != EOF) {
        count++;
    }

    FILE* out = fopen("count.txt", "w");
    if (!out) return 2;
    fprintf(out, "%d\n", count);
    fclose(out);

    printf("%d\n", count);
    return 0;
}

