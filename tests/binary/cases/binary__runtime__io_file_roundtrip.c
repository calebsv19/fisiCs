#include <stdio.h>

int main(void) {
    FILE* out = fopen("note.txt", "w");
    if (!out) return 2;
    fputs("hello\n", out);
    fclose(out);

    char buf[32] = {0};
    FILE* in = fopen("note.txt", "r");
    if (!in) return 3;
    if (!fgets(buf, sizeof(buf), in)) {
        fclose(in);
        return 4;
    }
    fclose(in);

    printf("%s", buf);
    return 0;
}

