#include <stdio.h>
#include <stdlib.h>

struct item {
    int key;
    int weight;
};

static int compare_items(const void *lhs, const void *rhs) {
    const struct item *a = (const struct item *)lhs;
    const struct item *b = (const struct item *)rhs;
    if (a->key < b->key) {
        return -1;
    }
    if (a->key > b->key) {
        return 1;
    }
    return a->weight - b->weight;
}

int main(void) {
    struct item values[5] = {
        {7, 2},
        {-3, 5},
        {11, -1},
        {0, 9},
        {4, 4},
    };
    struct item key = {4, 4};
    struct item missing = {6, 0};
    struct item *found;
    struct item *absent;
    int i;
    int checksum = 0;

    qsort(values, 5, sizeof(values[0]), compare_items);
    found = (struct item *)bsearch(&key, values, 5, sizeof(values[0]), compare_items);
    absent = (struct item *)bsearch(&missing, values, 5, sizeof(values[0]), compare_items);

    for (i = 0; i < 5; ++i) {
        checksum = checksum * 31 + values[i].key * 3 + values[i].weight;
    }

    printf("stdlib-sort first=%d:%d mid=%d:%d last=%d:%d found=%d absent=%d checksum=%d\n",
           values[0].key,
           values[0].weight,
           values[2].key,
           values[2].weight,
           values[4].key,
           values[4].weight,
           found ? found->weight : -99,
           absent == 0,
           checksum);

    return values[0].key == -3 && values[2].key == 4 && values[4].key == 11 &&
                   found != 0 && found->weight == 4 && absent == 0 && checksum == -3409844
               ? 0
               : 1;
}
