#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static int header_corpus_wave15_stdalign_summary(void) {
    struct HeaderCorpusWave15A {
        char c;
        int i;
    };
    struct HeaderCorpusWave15B {
        char c;
        long double ld;
    };

    return (int)_Alignof(struct HeaderCorpusWave15A) +
           (int)_Alignof(struct HeaderCorpusWave15B) +
           (int)_Alignof(int) +
           (int)_Alignof(long double) +
           (int)offsetof(struct HeaderCorpusWave15B, ld) +
           __alignas_is_defined +
           __alignof_is_defined;
}

static int header_corpus_wave15_overaligned_ok(void) {
    struct HeaderCorpusWave15Box {
        alignas(16) unsigned char bytes[16];
        int tail;
    };
    static struct HeaderCorpusWave15Box box = {{0}, 7};
    size_t box_align = _Alignof(struct HeaderCorpusWave15Box);
    size_t tail_offset = offsetof(struct HeaderCorpusWave15Box, tail);
    uintptr_t addr = (uintptr_t)(void *)&box.bytes[0];

    return box_align >= 16u &&
           (tail_offset % 16u) == 0u &&
           (addr % 16u) == 0u;
}

int main(void) {
    int summary = header_corpus_wave15_stdalign_summary();
    int overaligned_ok = header_corpus_wave15_overaligned_ok();

    if (summary != 27 || !overaligned_ok) {
        return 1;
    }

    printf("summary=%d over=%d\n", summary, overaligned_ok);
    return 0;
}
