#include <stdio.h>

int main(void) {
    unsigned char bytes[5] = {0u, 1u, 'A', 'B', 255u};
    unsigned char out[8] = {9u, 9u, 9u, 9u, 9u, 9u, 9u, 9u};
    char buffer[32];
    FILE *fp = tmpfile();
    size_t wrote = 0;
    size_t got = 0;
    int setbuf_rc = 0;
    int flush_rc = 0;
    long pos_after_write = 0;
    int eof_after_read = 0;
    int error_after_read = 0;
    int eof_after_clear = 0;
    int tail0 = 0;
    int tail1 = 0;
    long tail_pos = 0;
    int checksum = 0;
    int summary = 0;
    int i;

    if (!fp) {
        return 1;
    }

    setbuf_rc = setvbuf(fp, buffer, _IOFBF, sizeof(buffer));
    wrote = fwrite(bytes, 1, sizeof(bytes), fp);
    flush_rc = fflush(fp);
    pos_after_write = ftell(fp);
    rewind(fp);
    got = fread(out, 1, sizeof(out), fp);
    eof_after_read = feof(fp);
    error_after_read = ferror(fp);

    for (i = 0; i < 5; ++i) {
        checksum += (int)out[i] * (i + 1);
    }

    clearerr(fp);
    eof_after_clear = feof(fp);
    fseek(fp, -2L, SEEK_END);
    tail0 = fgetc(fp);
    tail1 = fgetc(fp);
    tail_pos = ftell(fp);
    fclose(fp);

    summary = (int)wrote + setbuf_rc + flush_rc + (int)pos_after_write +
              (int)got + eof_after_read + error_after_read + eof_after_clear +
              checksum + tail0 + tail1 + (int)tail_pos;

    printf("stdio-binary-buffer wrote=%lu setbuf=%d flush=%d pos=%ld got=%lu eof=%d err=%d clear=%d checksum=%d tail=%d/%d tailpos=%ld summary=%d\n",
           (unsigned long)wrote,
           setbuf_rc,
           flush_rc,
           pos_after_write,
           (unsigned long)got,
           eof_after_read,
           error_after_read,
           eof_after_clear,
           checksum,
           tail0,
           tail1,
           tail_pos,
           summary);

    return wrote == 5u && setbuf_rc == 0 && flush_rc == 0 &&
                   pos_after_write == 5L && got == 5u && eof_after_read != 0 &&
                   error_after_read == 0 && eof_after_clear == 0 &&
                   checksum == 1736 && tail0 == 'B' && tail1 == 255 &&
                   tail_pos == 5L && summary == 2078
               ? 0
               : 1;
}
