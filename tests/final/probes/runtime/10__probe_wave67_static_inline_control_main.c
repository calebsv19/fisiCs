#include <stdio.h>

#include "10__probe_wave67_static_inline_control_shared.h"

int main(void) {
    int local = wave67_static_adjust(3);
    int remote = wave67_static_from_lib(5);

    printf("%d %d\n", local, remote);
    return (local == 20 && remote == 34) ? 0 : 1;
}
