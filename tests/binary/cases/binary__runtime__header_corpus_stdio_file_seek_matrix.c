#include <stdio.h>

int main(void) {
    FILE *fp = tmpfile();
    int first;
    int third;
    int reread;
    int last;
    long pos;

    if (!fp) {
        return 2;
    }

    fputs("abcdef", fp);
    fflush(fp);
    rewind(fp);

    first = fgetc(fp);
    fseek(fp, 2L, SEEK_SET);
    third = fgetc(fp);
    ungetc(third, fp);
    pos = ftell(fp);
    reread = fgetc(fp);
    fseek(fp, 5L, SEEK_SET);
    last = fgetc(fp);
    fclose(fp);

    printf("stdio-file first=%d third=%d reread=%d last=%d pos=%ld\n",
           first,
           third,
           reread,
           last,
           pos);
    return first == 'a' && third == 'c' && reread == 'c' && last == 'f' && pos == 2L ? 0 : 1;
}
