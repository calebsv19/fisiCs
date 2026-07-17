#include <stdio.h>

struct fixed_packet {
    unsigned int tag;
    unsigned short count;
    unsigned char payload[6];
};

int main(void) {
    struct fixed_packet source;
    struct fixed_packet destination;
    unsigned int source_sum = 0U;
    unsigned int destination_sum = 0U;
    unsigned int index;

    source.tag = 0x1234U;
    source.count = 6U;
    destination.tag = 0xabcdU;
    destination.count = 2U;

    for (index = 0U; index < 6U; ++index) {
        source.payload[index] = (unsigned char)(10U + index);
        destination.payload[index] = (unsigned char)(90U + index);
    }

    destination = source;

    for (index = 0U; index < 6U; ++index) {
        source_sum += source.payload[index];
        destination_sum += destination.payload[index];
    }

    printf("%u %u %u %u %u\n",
           destination.tag,
           (unsigned int)destination.count,
           source_sum,
           destination_sum,
           (unsigned int)destination.payload[0]);

    return destination.tag == 0x1234U &&
                   destination.count == 6U &&
                   source_sum == 75U &&
                   destination_sum == 75U &&
                   destination.payload[0] == 10U
               ? 0
               : 1;
}
