#include <stdio.h>

int main(void) {
    FILE *fp = tmpfile();
    int first;
    int second;
    int eof_before;
    int err_before;
    int eof_after;
    int err_after;
    long pos;

    if (!fp) {
        return 2;
    }

    fputs("xy", fp);
    fflush(fp);
    rewind(fp);

    first = fgetc(fp);
    second = fgetc(fp);
    eof_before = feof(fp);
    (void)fgetc(fp);
    eof_after = feof(fp);
    err_before = ferror(fp);
    clearerr(fp);
    err_after = ferror(fp) + feof(fp);
    pos = ftell(fp);
    fclose(fp);

    printf("stdio-state first=%d second=%d eof_before=%d eof_after=%d err_before=%d err_after=%d pos=%ld\n",
           first,
           second,
           eof_before,
           eof_after,
           err_before,
           err_after,
           pos);

    return first == 'x' && second == 'y' && eof_before == 0 && eof_after != 0 &&
                   err_before == 0 && err_after == 0 && pos == 2L
               ? 0
               : 1;
}
