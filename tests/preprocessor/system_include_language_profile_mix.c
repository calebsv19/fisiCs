#include "shadow.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

int main(void) {
    uint32_t local = (uint32_t)SHADOW_VAL;
    size_t tsz = sizeof(time_t);
    bool ok = (local == 7u) && (tsz >= 1u);
    return ok ? (int)local : 0;
}
