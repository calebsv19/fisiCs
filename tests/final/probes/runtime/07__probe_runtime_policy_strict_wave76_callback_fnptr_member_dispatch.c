#include <stdio.h>

enum Tag {
    TAG_LOW = 2,
    TAG_HIGH = 129
};

struct Item {
    enum Tag tag;
    unsigned char raw;
    signed char adj;
};

typedef int (*item_fn)(struct Item *, unsigned short);

static int calc_wide(struct Item *item, unsigned short salt) {
    unsigned int x = (unsigned int)(unsigned char)(item->raw + (unsigned char)salt);
    return (int)(x & 127u) + item->tag + item->adj;
}

static int calc_signed(struct Item *item, unsigned short salt) {
    int signed_lane = (int)(signed char)(item->raw - (unsigned char)item->tag);
    return signed_lane + (int)(unsigned char)(salt + item->adj);
}

static item_fn choose(item_fn *table, enum Tag tag, int flip) {
    item_fn first = tag > 100 ? table[1] : table[0];
    return flip ? table[0] : first;
}

static int run(item_fn (*chooser)(item_fn *, enum Tag, int), item_fn *table, struct Item *item, unsigned short salt, int flip) {
    item_fn selected = chooser(table, item->tag, flip);
    item_fn fallback = selected ? selected : calc_wide;
    return fallback(item, salt);
}

int main(void) {
    struct Item items[3] = {
        {TAG_LOW, 240u, -5},
        {TAG_HIGH, 33u, 7},
        {TAG_HIGH, 250u, -9}
    };
    item_fn table[2] = {calc_wide, calc_signed};
    item_fn (*chooser)(item_fn *, enum Tag, int) = choose;

    int first = run(chooser, table, &items[0], 9u, 0);
    int second = run(chooser, table, &items[1], 5u, 0);
    int third = run(chooser, table, &items[2], 11u, 0);
    printf("%d %d %d\n", first, second, third);
    return 0;
}
