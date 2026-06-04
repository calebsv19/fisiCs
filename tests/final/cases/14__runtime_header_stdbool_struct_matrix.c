#include <stdbool.h>
#include <stdio.h>

struct Wave21BoolCell {
    bool active;
    bool dirty;
    int weight;
};

int main(void) {
    struct Wave21BoolCell cells[3] = {
        { true, false, 3 },
        { false, true, 5 },
        { true, true, 7 }
    };
    int total = 0;
    int i;

    for (i = 0; i < 3; ++i) {
        if (cells[i].active) {
            total += cells[i].weight * 10;
        }
        if (cells[i].dirty) {
            total += cells[i].weight;
        }
    }

    printf("stdbool-struct total=%d size=%lu\n", total, (unsigned long)sizeof(cells[0]));
    return total == 112 ? 0 : 1;
}
