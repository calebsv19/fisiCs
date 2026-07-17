#include <stddef.h>
#include <stdio.h>

union Word {
    unsigned char bytes[6];
    char text[6];
};

struct Entry {
    unsigned char id;
    union Word words[2];
    unsigned char flags[2];
};

struct Table {
    struct Entry entries[3];
    union Word trailer;
};

static unsigned checksum(const struct Table *table) {
    unsigned acc = table->trailer.bytes[0] + table->trailer.bytes[5];

    for (int i = 0; i < 3; ++i) {
        const struct Entry *entry = &table->entries[i];
        acc = acc * 43u + entry->id;
        for (int w = 0; w < 2; ++w) {
            acc = acc * 17u + (unsigned char)entry->words[w].text[0];
            acc = acc * 13u + (unsigned char)entry->words[w].text[2];
            acc = acc * 11u + entry->words[w].bytes[5];
        }
        acc = acc * 7u + entry->flags[0];
        acc = acc * 5u + entry->flags[1];
    }

    return acc;
}

int main(void) {
    struct Table table = {
        .entries[0] = {
            .id = 1,
            .words = {
                [0].text = "ab",
                [1].bytes = { 'c', 'd', 0, 'e', 0, 0 },
            },
            .flags = { 2, 3 },
        },
        .entries[1].words[1].text = "xy",
        .entries[1].words[1].bytes[2] = 'z',
        .entries[1].flags[1] = 5,
        .entries[2] = {
            .id = 8,
            .words[0].text = "hi",
            .words[1].bytes = { 9, 10, 11, 12, 13, 14 },
            .flags[0] = 21,
        },
        .entries[2].words[0].bytes[5] = 34,
        .trailer.text = "ok",
        .trailer.bytes[5] = 55,
    };

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Table, entries),
           (unsigned)offsetof(struct Entry, words),
           (unsigned)offsetof(struct Entry, flags),
           (unsigned)table.entries[0].words[0].bytes[2],
           (unsigned)table.entries[1].words[0].bytes[0],
           (unsigned)table.entries[2].words[0].bytes[5],
           checksum(&table));
    return 0;
}
