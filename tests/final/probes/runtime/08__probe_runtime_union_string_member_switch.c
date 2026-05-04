#include <stdio.h>

union Payload {
    char text[5];
    unsigned raw;
};

struct Entry {
    int kind;
    union Payload payload;
    char label[4];
};

static unsigned score(const struct Entry *entry) {
    return (unsigned)entry->kind * 100u
        + (unsigned char)entry->payload.text[0]
        + (unsigned char)entry->payload.text[2]
        + (unsigned char)entry->label[1]
        + (unsigned char)entry->label[3];
}

int main(void) {
    struct Entry entries[2] = {
        {
            .kind = 1,
            .payload.text = "ab",
            .label = "xy",
        },
        {
            .label = "z",
            .payload.text = "io",
            .kind = 2,
        },
    };

    printf(
        "%u %u %u %u\n",
        (unsigned char)entries[0].payload.text[3],
        (unsigned char)entries[1].payload.text[2],
        (unsigned char)entries[1].label[1],
        score(&entries[1])
    );
    return 0;
}
