#include <stdio.h>

enum Mode {
    MODE_SUM = 1,
    MODE_DIFF = 4,
    MODE_MIX = 9
};

struct Pair {
    unsigned char left;
    signed char right;
};

struct Payload {
    enum Mode mode;
    struct Pair pairs[2];
    unsigned short salt;
};

typedef int (*payload_handler)(struct Payload *, unsigned char);

static int handle_sum(struct Payload *payload, unsigned char lane) {
    struct Pair *pair = &payload->pairs[lane & 1u];
    return (int)(unsigned char)(pair->left + (unsigned char)payload->mode) +
           (int)pair->right + (int)(payload->salt & 31u);
}

static int handle_diff(struct Payload *payload, unsigned char lane) {
    struct Pair *pair = &payload->pairs[(lane + 1u) & 1u];
    return (int)(unsigned char)(pair->left - (unsigned char)payload->mode) -
           (int)(signed char)(pair->right + (signed char)(payload->salt & 7u));
}

static int handle_mix(struct Payload *payload, unsigned char lane) {
    struct Pair *pair = &payload->pairs[(unsigned int)(payload->mode + lane) & 1u];
    unsigned int mixed = (unsigned int)(unsigned char)(pair->left + (unsigned char)pair->right);
    return (int)(mixed ^ (unsigned int)(payload->salt & 63u)) + (int)payload->mode;
}

static int dispatch(struct Payload *payload, payload_handler fallback) {
    payload_handler table[3] = {handle_sum, handle_diff, handle_mix};
    unsigned char index = (unsigned char)((int)payload->mode % 3);
    payload_handler chosen = index == 1u ? fallback : table[index];
    unsigned char lane = (unsigned char)(payload->pairs[0].left + (unsigned char)payload->mode);
    return chosen(payload, lane);
}

int main(void) {
    struct Payload payloads[3] = {
        {MODE_SUM, {{240u, -5}, {18u, 7}}, 23u},
        {MODE_DIFF, {{91u, 12}, {205u, -19}}, 38u},
        {MODE_MIX, {{33u, 21}, {144u, -8}}, 55u}
    };

    int first = dispatch(&payloads[0], handle_mix);
    int second = dispatch(&payloads[1], handle_diff);
    int third = dispatch(&payloads[2], handle_sum);
    printf("%d %d %d %d\n", first, second, third, first - second + third);
    return 0;
}
