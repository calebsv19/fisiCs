#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>

static int wave353_token = 53;

struct Wave353Selected {
    int *token;
    int tag;
};

struct Wave353Carrier {
    uint64_t lanes[3];
    int *tail;
};

union Wave353Union {
    alignas(64) struct Wave353Selected selected;
    struct Wave353Carrier carrier;
    unsigned char bytes[64];
};

static union Wave353Union local_values[2] = {
    {.selected = {&wave353_token, 11}},
    {.carrier = {{17, 19, 23}, &wave353_token}},
};

int wave353_local_value(void) {
    return *local_values[0].selected.token;
}

int wave353_local_pointer_matches(void) {
    return local_values[0].selected.token == &wave353_token;
}

size_t wave353_local_stride(void) {
    return (size_t)((uintptr_t)&local_values[1] -
                    (uintptr_t)&local_values[0]);
}
