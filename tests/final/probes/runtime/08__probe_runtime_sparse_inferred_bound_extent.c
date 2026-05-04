#include <stdio.h>

int main(void) {
    int values[] = {
        [4] = 9,
        [1] = 3,
    };
    size_t count = sizeof(values) / sizeof(values[0]);
    int checksum = 0;

    for (size_t i = 0; i < count; ++i) {
        checksum = checksum * 7 + values[i];
    }

    printf("%zu %d %d %d\n", count, values[0], values[1], checksum);
    return 0;
}
