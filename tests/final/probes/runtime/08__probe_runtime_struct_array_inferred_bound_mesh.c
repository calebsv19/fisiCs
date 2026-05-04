#include <stdio.h>

struct Row {
    int cells[3];
};

int main(void) {
    struct Row rows[] = {
        [2] = {
            .cells = {
                [1] = 4,
            },
        },
        [0] = {
            .cells = {1, 2, 3},
        },
    };
    size_t count = sizeof(rows) / sizeof(rows[0]);
    int checksum = 0;

    for (size_t i = 0; i < count; ++i) {
        for (int j = 0; j < 3; ++j) {
            checksum = checksum * 5 + rows[i].cells[j];
        }
    }

    printf("%zu %d %d %d\n", count, rows[1].cells[1], rows[2].cells[1], checksum);
    return 0;
}
