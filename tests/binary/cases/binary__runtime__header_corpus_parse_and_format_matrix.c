#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HeaderCorpusLayout {
    uint32_t prefix;
    char payload[8];
};

int main(void) {
    const char* raw = "ff:done";
    char* end = 0;
    unsigned long value = strtoul(raw, &end, 16);
    struct HeaderCorpusLayout layout;
    int offset = (int)offsetof(struct HeaderCorpusLayout, payload);
    int len = 0;

    memcpy(layout.payload, "done", 5);
    len = (int)strlen(layout.payload);

    if (value != 255ul || !end || *end != ':' || strcmp(layout.payload, "done") != 0) {
        return 1;
    }
    if (offset != 4 || len != 4 || UINT8_MAX != 255u || CHAR_BIT != 8) {
        return 2;
    }

    printf(
        "value=%lu offset=%d len=%d tail=%s max=%u bits=%d\n",
        value,
        offset,
        len,
        end + 1,
        (unsigned)UINT8_MAX,
        CHAR_BIT);
    return 0;
}
