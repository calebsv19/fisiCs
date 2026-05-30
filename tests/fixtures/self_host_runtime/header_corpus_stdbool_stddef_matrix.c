#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct HeaderCorpusWave5Pair {
    int left;
    int right;
};

static bool header_corpus_wave5_span_summary(
    const char *text,
    char *tail,
    size_t tail_cap,
    size_t *prefix_out,
    size_t *digit_span_out) {
    static const char digits[] = "0123456789";
    size_t prefix = 0;
    size_t digit_span = 0;

    if (!text || !tail || tail_cap < 5U || !prefix_out || !digit_span_out) {
        return false;
    }

    prefix = strcspn(text, "-");
    digit_span = strspn(text + 6, digits);
    memmove(tail, text + 9, 4U);
    tail[4] = '\0';
    *prefix_out = prefix;
    *digit_span_out = digit_span;
    return strcmp(tail, "beta") == 0;
}

int main(void) {
    struct HeaderCorpusWave5Pair pairs[4] = {
        {1, 2},
        {3, 4},
        {5, 6},
        {7, 8},
    };
    char tail[8] = {0};
    ptrdiff_t delta = &pairs[3] - &pairs[0];
    size_t bytes = sizeof(pairs);
    size_t offset = offsetof(struct HeaderCorpusWave5Pair, right);
    size_t prefix = 0;
    size_t digit_span = 0;
    bool span_ok = false;
    bool ordered = offsetof(struct HeaderCorpusWave5Pair, left) < offset;

    span_ok = header_corpus_wave5_span_summary(
        "alpha-42-beta",
        tail,
        sizeof(tail),
        &prefix,
        &digit_span);

    if (!span_ok || prefix != 5U || digit_span != 2U) {
        return 1;
    }
    if (!ordered || delta != 3) {
        return 2;
    }

    printf(
        "prefix=%zu digits=%zu tail=%s delta=%td bytes=%zu offset=%zu ordered=%d\n",
        prefix,
        digit_span,
        tail,
        delta,
        bytes,
        offset,
        ordered ? 1 : 0);
    return 0;
}
