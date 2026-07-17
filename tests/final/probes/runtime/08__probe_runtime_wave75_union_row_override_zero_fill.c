#include <stddef.h>
#include <stdio.h>

union Payload {
    unsigned char raw[8];
    struct {
        unsigned char opcode;
        unsigned char lane;
        unsigned char data[6];
    } msg;
    unsigned short half[4];
};

struct Packet {
    union Payload payloads[3];
    unsigned char flags[2];
};

struct Batch {
    struct Packet packets[2];
};

static unsigned checksum(const struct Batch *batch) {
    unsigned acc = 0;

    for (int p = 0; p < 2; ++p) {
        const struct Packet *packet = &batch->packets[p];
        acc = acc * 43u + packet->flags[0];
        acc = acc * 37u + packet->flags[1];
        for (int i = 0; i < 3; ++i) {
            const union Payload *payload = &packet->payloads[i];
            acc = acc * 31u + payload->raw[0];
            acc = acc * 29u + payload->raw[1];
            acc = acc * 23u + payload->raw[7];
        }
    }

    return acc;
}

int main(void) {
    struct Batch batch = {
        .packets[0].payloads[0].msg = { .opcode = 3, .lane = 5, .data = { [5] = 7 } },
        .packets[0].payloads[1].raw = { 11, 13, 17 },
        .packets[0].payloads[1].half[3] = 19,
        .packets[0].flags[1] = 23,
        .packets[1] = {
            .payloads = {
                [0].half = { 29, 31 },
                [2].msg = { .opcode = 37, .lane = 41, .data = { [4] = 43 } },
            },
            .flags = { 47, 53 },
        },
        .packets[1].payloads[2].raw = { 59, 61 },
        .packets[1].payloads[2].msg.data[5] = 67,
    };

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Batch, packets),
           (unsigned)offsetof(struct Packet, payloads),
           (unsigned)offsetof(struct Packet, flags),
           (unsigned)batch.packets[0].payloads[2].raw[0],
           (unsigned)batch.packets[0].payloads[1].raw[7],
           (unsigned)batch.packets[1].payloads[2].raw[0],
           checksum(&batch));
    return 0;
}
