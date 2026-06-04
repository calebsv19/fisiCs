#include <stdio.h>

int wave25_stdio_position_surface(FILE *fp) {
    fpos_t pos;
    int total = 0;

    total += fgetpos(fp, &pos);
    total += fsetpos(fp, &pos);
    total += fseek(fp, 0L, SEEK_SET);
    total += (int)ftell(fp);
    return total;
}

int main(void) {
    return 0;
}
