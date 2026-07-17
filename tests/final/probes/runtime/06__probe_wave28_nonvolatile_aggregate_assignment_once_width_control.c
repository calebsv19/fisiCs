#include <stdio.h>

typedef struct {
    unsigned char lead;
    unsigned int lane[3];
    unsigned char tail;
} Wave28Payload;

typedef struct {
    unsigned int before;
    Wave28Payload payload;
    unsigned int after;
} Wave28Guarded;

static Wave28Guarded source = {
    0x11111111u,
    {0xa1u, {101u, 202u, 303u}, 7u},
    0x22222222u
};

static Wave28Guarded target = {
    0x33333333u,
    {0xb2u, {404u, 505u, 606u}, 9u},
    0x44444444u
};

static int target_evaluations;
static int source_evaluations;

static Wave28Payload *select_target(void) {
    ++target_evaluations;
    return &target.payload;
}

static Wave28Payload *select_source(void) {
    ++source_evaluations;
    return &source.payload;
}

int main(void) {
    *select_target() = *select_source();

    printf("%d %d %x %u %u %u %u %x %x %x %u\n",
           target_evaluations,
           source_evaluations,
           (unsigned int)target.payload.lead,
           target.payload.lane[0],
           target.payload.lane[1],
           target.payload.lane[2],
           (unsigned int)target.payload.tail,
           target.before,
           target.after,
           source.before,
           source.payload.lane[2]);
    return 0;
}
