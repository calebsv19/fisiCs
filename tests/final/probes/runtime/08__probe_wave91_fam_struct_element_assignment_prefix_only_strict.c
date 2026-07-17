#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct cell {
    unsigned short left;
    unsigned short right;
};

struct cell_packet {
    unsigned int generation;
    unsigned int count;
    struct cell cells[];
};

typedef char cell_packet_has_no_trailing_padding[
    sizeof(struct cell_packet) == offsetof(struct cell_packet, cells) ? 1 : -1
];

int main(void) {
    const unsigned int cell_count = 3U;
    struct cell_packet *source = (struct cell_packet *)malloc(
        sizeof(struct cell_packet) + cell_count * sizeof(struct cell));
    struct cell_packet *destination = (struct cell_packet *)malloc(
        sizeof(struct cell_packet) + cell_count * sizeof(struct cell));
    unsigned int source_sum = 0U;
    unsigned int destination_sum = 0U;
    unsigned int index;

    if (source == NULL || destination == NULL) {
        free(source);
        free(destination);
        return 2;
    }

    source->generation = 41U;
    source->count = cell_count;
    destination->generation = 9U;
    destination->count = 1U;

    for (index = 0U; index < cell_count; ++index) {
        source->cells[index].left = (unsigned short)(10U + index);
        source->cells[index].right = (unsigned short)(20U + index);
        destination->cells[index].left = (unsigned short)(70U + index);
        destination->cells[index].right = (unsigned short)(80U + index);
    }

    *destination = *source;

    for (index = 0U; index < cell_count; ++index) {
        source_sum += source->cells[index].left + source->cells[index].right;
        destination_sum +=
            destination->cells[index].left + destination->cells[index].right;
    }

    printf("%u %u %u %u %u %u\n",
           destination->generation,
           destination->count,
           source_sum,
           destination_sum,
           (unsigned int)destination->cells[0].left,
           (unsigned int)destination->cells[2].right);

    index = destination->generation == 41U &&
                    destination->count == cell_count &&
                    source_sum == 96U &&
                    destination_sum == 456U &&
                    destination->cells[0].left == 70U &&
                    destination->cells[2].right == 82U
                ? 0U
                : 1U;

    free(source);
    free(destination);
    return (int)index;
}
