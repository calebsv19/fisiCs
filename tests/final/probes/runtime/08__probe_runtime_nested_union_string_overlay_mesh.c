#include <stdio.h>

union Payload {
    char text[6];
    struct {
        unsigned char a;
        unsigned char b;
        unsigned char c;
        unsigned char d;
        unsigned char e;
        unsigned char f;
    } bytes;
};

struct Item {
    int kind;
    union Payload payload;
    char name[4];
};

static unsigned score(const struct Item *item) {
    return (unsigned)item->kind * 100u
        + (unsigned char)item->payload.text[0]
        + (unsigned char)item->payload.text[2]
        + (unsigned char)item->name[1]
        + (unsigned char)item->name[3];
}

int main(void) {
    struct Item items[3] = {
        [0] = {
            .kind = 1,
            .payload.text = "ok",
            .name = "m",
        },
        [1] = {
            .payload.text = "ab",
            .name = "q",
        },
        [2] = {
            .name = "io",
            .payload.text = "xyz",
            .kind = 4,
        },
    };

    printf(
        "%u %u %u %u\n",
        (unsigned char)items[1].payload.text[2],
        (unsigned char)items[2].payload.text[3],
        (unsigned char)items[2].name[2],
        score(&items[2])
    );
    return 0;
}
