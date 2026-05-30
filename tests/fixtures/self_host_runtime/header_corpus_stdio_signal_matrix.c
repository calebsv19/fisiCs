#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static jmp_buf g_header_corpus_wave7_env;
static volatile sig_atomic_t g_header_corpus_wave7_signal_count = 0;

static void header_corpus_wave7_on_signal(int signum) {
    if (signum == SIGINT) {
        g_header_corpus_wave7_signal_count += 1;
    }
}

static bool header_corpus_wave7_stdio_signal_summary(
    FILE *stream,
    const char *text,
    size_t *len_out,
    int *pos_out
) {
    fpos_t checkpoint;
    size_t len = 0U;
    long end_pos = 0L;

    if (!stream || !text || !len_out || !pos_out) {
        return false;
    }

    if (setjmp(g_header_corpus_wave7_env) != 0) {
        return false;
    }

    clearerr(stream);
    if (fgetpos(stream, &checkpoint) != 0) {
        return false;
    }

    len = strlen(text);
    if (fwrite(text, 1U, len, stream) != len) {
        return false;
    }

    end_pos = ftell(stream);
    header_corpus_wave7_on_signal(SIGINT);
    if (g_header_corpus_wave7_signal_count != 1) {
        return false;
    }

    *len_out = len;
    *pos_out = (int)end_pos;
    return true;
}

int main(void) {
    FILE *stream = tmpfile();
    size_t len = 0U;
    int pos = 0;
    bool ok = false;

    if (!stream) {
        return 1;
    }

    ok = header_corpus_wave7_stdio_signal_summary(
        stream,
        "wave7-signal",
        &len,
        &pos);
    fclose(stream);

    if (!ok || len != (size_t)12U || pos != 12) {
        return 2;
    }

    printf(
        "len=%zu pos=%d signals=%d\n",
        len,
        pos,
        (int)g_header_corpus_wave7_signal_count);
    return 0;
}
