#include <stdint.h>
#include <stdio.h>
#include <stdalign.h>

int wave353_token = 29;

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

static union Wave353Union main_values[2] = {
    {.selected = {&wave353_token, 7}},
    {.carrier = {{11, 13, 17}, &wave353_token}},
};

extern int wave353_local_value(void);
extern int wave353_local_pointer_matches(void);
extern size_t wave353_local_stride(void);

int main(void) {
    size_t main_stride = (size_t)((uintptr_t)&main_values[1] -
                                  (uintptr_t)&main_values[0]);
    printf("%zu %zu %d %d %d %d %zu %zu\n",
           alignof(union Wave353Union),
           sizeof(union Wave353Union),
           main_values[0].selected.token == &wave353_token,
           *main_values[0].selected.token,
           wave353_local_pointer_matches(),
           wave353_local_value(),
           main_stride,
           wave353_local_stride());
    return alignof(union Wave353Union) == 64 &&
                   sizeof(union Wave353Union) == 64 &&
                   main_values[0].selected.token == &wave353_token &&
                   *main_values[0].selected.token == 29 &&
                   wave353_local_pointer_matches() == 1 &&
                   wave353_local_value() == 53 && main_stride == 64 &&
                   wave353_local_stride() == 64
               ? 0
               : 1;
}
