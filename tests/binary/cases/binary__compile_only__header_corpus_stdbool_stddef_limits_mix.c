#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

struct HeaderCorpusWave5Span {
    bool ok;
    size_t count;
    ptrdiff_t delta;
};

int header_corpus_wave5_span_score(
    const struct HeaderCorpusWave5Span *span,
    const char *first,
    const char *last) {
    ptrdiff_t width = 0;

    if (!span || !first || !last || last < first) {
        return -1;
    }

    width = last - first;
    return span->ok ? (int)(span->count + (size_t)width + (size_t)CHAR_BIT) : (int)span->delta;
}
