#include <stdio.h>

int main(void) {
    FILE *fp = tmpfile();
    int a = 0;
    int b = 0;
    int c = 0;
    int eof_value = 0;
    int feof_before = 0;
    int feof_after = 0;
    int ferror_after = 0;
    int pushed = 0;
    int z = 0;
    int again = 0;
    long pos = 0;
    int summary = 0;

    if (!fp) {
        return 1;
    }

    fputs("abc", fp);
    rewind(fp);
    a = fgetc(fp);
    b = fgetc(fp);
    c = fgetc(fp);
    eof_value = fgetc(fp);
    feof_before = feof(fp);
    clearerr(fp);
    feof_after = feof(fp);
    ferror_after = ferror(fp);

    fseek(fp, 0, SEEK_SET);
    pushed = ungetc('Z', fp);
    z = fgetc(fp);
    again = fgetc(fp);
    pos = ftell(fp);
    fclose(fp);

    summary = a + b + c + (eof_value == EOF ? 17 : 0) + feof_before +
              feof_after + ferror_after + pushed + z + again + (int)pos;

    printf("stdio-stream-ungetc chars=%c%c%c eof=%d feof=%d clear=%d/%d push=%c got=%c next=%c pos=%ld summary=%d\n",
           a,
           b,
           c,
           eof_value == EOF ? 1 : 0,
           feof_before,
           feof_after,
           ferror_after,
           pushed,
           z,
           again,
           pos,
           summary);

    return a == 'a' && b == 'b' && c == 'c' && eof_value == EOF &&
                   feof_before != 0 && feof_after == 0 && ferror_after == 0 &&
                   pushed == 'Z' && z == 'Z' && again == 'a' && pos == 1L &&
                   summary == 590
               ? 0
               : 1;
}
