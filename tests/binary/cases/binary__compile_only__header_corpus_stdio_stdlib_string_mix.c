#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HeaderCorpusRecord {
    uint8_t byte_value;
    size_t length;
    ptrdiff_t delta;
    uintptr_t token;
    char text[8];
};

static int header_corpus_accepts_mix(const struct HeaderCorpusRecord* record, FILE* stream) {
    char local[16];
    size_t span = strlen(record->text);
    int written = snprintf(local, sizeof(local), "%zu:%u", span, (unsigned)record->byte_value);
    if (written < 0 || !stream) {
        return EXIT_FAILURE;
    }
    return (int)(CHAR_BIT + (int)(record->delta != 0) + (int)(local[0] != '\0'));
}

int main(void) {
    struct HeaderCorpusRecord record;
    record.byte_value = UINT8_MAX;
    record.length = sizeof(record.text);
    record.delta = (ptrdiff_t)offsetof(struct HeaderCorpusRecord, text);
    record.token = (uintptr_t)(record.length + (size_t)record.delta);
    memcpy(record.text, "mix", 4);
    return header_corpus_accepts_mix(&record, stdout) == 0 ? EXIT_SUCCESS : EXIT_SUCCESS;
}
