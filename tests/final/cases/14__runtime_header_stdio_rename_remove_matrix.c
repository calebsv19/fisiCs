#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *src = "fisics_stdio_wave318_src.tmp";
    const char *dst = "fisics_stdio_wave318_dst.tmp";
    char buf[32] = {0};
    FILE *fp = 0;
    FILE *check = 0;
    int rename_rc = -1;
    int remove_rc = -1;
    int missing_remove_rc = 0;
    int missing_errno = 0;
    int missing_open = 0;
    size_t got = 0;
    int summary = 0;

    remove(src);
    remove(dst);

    fp = fopen(src, "wb");
    if (!fp) {
        return 1;
    }
    fputs("rename-data", fp);
    fclose(fp);

    rename_rc = rename(src, dst);
    check = fopen(dst, "rb");
    if (!check) {
        remove(src);
        remove(dst);
        return 2;
    }
    got = fread(buf, 1, sizeof(buf) - 1u, check);
    fclose(check);

    remove_rc = remove(dst);
    check = fopen(dst, "rb");
    missing_open = check == 0 ? 1 : 0;
    if (check) {
        fclose(check);
    }

    errno = 0;
    missing_remove_rc = remove(dst);
    missing_errno = errno;

    summary = rename_rc + remove_rc + missing_open +
              (missing_remove_rc != 0 ? 1 : 0) +
              (missing_errno == ENOENT ? 1 : 0) + (int)got +
              (int)buf[0] + (int)buf[10];

    printf("stdio-rename-remove rename=%d remove=%d missing_open=%d missing_remove=%d errno=%d got=%lu text=%s summary=%d\n",
           rename_rc,
           remove_rc,
           missing_open,
           missing_remove_rc != 0 ? 1 : 0,
           missing_errno,
           (unsigned long)got,
           buf,
           summary);

    return rename_rc == 0 && remove_rc == 0 && missing_open == 1 &&
                   missing_remove_rc != 0 && missing_errno == ENOENT &&
                   got == 11u && strcmp(buf, "rename-data") == 0 &&
                   summary == 225
               ? 0
               : 1;
}
