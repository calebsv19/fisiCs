#include <stdio.h>
#include <string.h>

int main(void) {
    static const char payload[] = "alpha-beta";
    char buffer[32];
    FILE *stream = tmpfile();
    size_t written = 0U;
    size_t read_count = 0U;
    long end_pos = 0L;
    int eof_after_read = 0;
    int err_after_read = 0;

    if (!stream) {
        return 1;
    }

    written = fwrite(payload, 1U, strlen(payload), stream);
    if (written != strlen(payload)) {
        fclose(stream);
        return 2;
    }

    end_pos = ftell(stream);
    if (end_pos != (long)strlen(payload)) {
        fclose(stream);
        return 3;
    }

    if (fseek(stream, 0L, SEEK_SET) != 0) {
        fclose(stream);
        return 4;
    }

    memset(buffer, 0, sizeof(buffer));
    read_count = fread(buffer, 1U, sizeof(buffer) - 1U, stream);
    eof_after_read = feof(stream);
    err_after_read = ferror(stream);
    fclose(stream);

    if (read_count != strlen(payload) || strcmp(buffer, payload) != 0) {
        return 5;
    }
    if (!eof_after_read || err_after_read != 0) {
        return 6;
    }

    printf(
        "written=%zu pos=%ld text=%s eof=%d err=%d\n",
        written,
        end_pos,
        buffer,
        eof_after_read,
        err_after_read);
    return 0;
}
