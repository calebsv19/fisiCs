#include <stdio.h>

#include "03__probe_runtime_wave38_guard_collision_a.h"
#include "03__probe_runtime_wave38_guard_collision_b.h"

#ifndef W38_COLLISION_SECOND_VALUE
#define W38_COLLISION_SECOND_VALUE -7
#endif

int main(void) {
    printf("%d %d\n", W38_COLLISION_FIRST_VALUE, W38_COLLISION_SECOND_VALUE);
    return 0;
}
