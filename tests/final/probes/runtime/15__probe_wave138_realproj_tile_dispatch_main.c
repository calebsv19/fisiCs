#include <stdio.h>

#include "15__probe_wave138_realproj_tile_dispatch_shared.h"

int main(void) {
    Wave138TileRow rows[4] = {
        {41u, 1u, 0u, 1u, 7u, 3u},
        {77u, 2u, 1u, 2u, 11u, 5u},
        {95u, 1u, 2u, 0u, 5u, 7u},
        {109u, 0u, 3u, 3u, 9u, 11u},
    };
    Wave138StepFn steps[3] = {
        wave138_step_update,
        wave138_step_owner,
        wave138_step_reclaim,
    };
    unsigned checksum = wave138_tile_dispatch_checksum(rows, steps);

    printf("%u\n", checksum);
    return 0;
}
