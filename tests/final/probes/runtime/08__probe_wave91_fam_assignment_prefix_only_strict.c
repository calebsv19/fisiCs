#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct packet {
    unsigned int tag;
    unsigned int count;
    unsigned char payload[];
};

typedef char packet_has_no_trailing_padding[
    sizeof(struct packet) == offsetof(struct packet, payload) ? 1 : -1
];

int main(void) {
    const unsigned int tail_count = 6U;
    struct packet *source =
        (struct packet *)malloc(sizeof(struct packet) + tail_count);
    struct packet *destination =
        (struct packet *)malloc(sizeof(struct packet) + tail_count);
    unsigned int source_sum = 0U;
    unsigned int destination_sum = 0U;
    unsigned int index;

    if (source == NULL || destination == NULL) {
        free(source);
        free(destination);
        return 2;
    }

    source->tag = 0x1234U;
    source->count = tail_count;
    destination->tag = 0xabcdU;
    destination->count = 2U;

    for (index = 0U; index < tail_count; ++index) {
        source->payload[index] = (unsigned char)(10U + index);
        destination->payload[index] = (unsigned char)(90U + index);
    }

    *destination = *source;

    for (index = 0U; index < tail_count; ++index) {
        source_sum += source->payload[index];
        destination_sum += destination->payload[index];
    }

    printf("%u %u %u %u %u\n",
           destination->tag,
           (unsigned int)destination->count,
           source_sum,
           destination_sum,
           (unsigned int)destination->payload[0]);

    index = destination->tag == 0x1234U &&
                    destination->count == tail_count &&
                    source_sum == 75U &&
                    destination_sum == 555U &&
                    destination->payload[0] == 90U
                ? 0U
                : 1U;

    free(source);
    free(destination);
    return (int)index;
}
