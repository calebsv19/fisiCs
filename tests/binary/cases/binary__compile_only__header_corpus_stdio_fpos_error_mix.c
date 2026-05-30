#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static bool header_corpus_wave7_prepare_stream(
    FILE *stream,
    fpos_t *position,
    size_t limit
) {
    if (!position || limit == 0U) {
        errno = EINVAL;
        return false;
    }

    if (stream) {
        clearerr(stream);
    }

    (void)position;
    return true;
}

int main(void) {
    fpos_t position;
    bool ok = header_corpus_wave7_prepare_stream(0, &position, (size_t)4U);

    return ok ? 0 : 1;
}
