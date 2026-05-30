#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct HeaderCorpusWave5Pair {
    int left;
    int right;
};

int main(void) {
    struct HeaderCorpusWave5Pair pairs[4] = {
        {1, 2},
        {3, 4},
        {5, 6},
        {7, 8},
    };
    ptrdiff_t delta = &pairs[3] - &pairs[0];
    size_t bytes = sizeof(pairs);
    size_t offset = offsetof(struct HeaderCorpusWave5Pair, right);
    bool ordered = offsetof(struct HeaderCorpusWave5Pair, left) < offset;

    printf(
        "delta=%td bytes=%zu offset=%zu ordered=%d\n",
        delta,
        bytes,
        offset,
        ordered ? 1 : 0);
    return ordered && delta == 3 ? 0 : 1;
}
