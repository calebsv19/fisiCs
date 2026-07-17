#include <stdio.h>

#include "10__probe_wave67_external_inline_shared.h"

int main(void) {
    int direct = wave67_external_adjust(2);
    int provider = wave67_external_from_provider(4);
    int observer = wave67_external_from_observer(6);

    printf("%d %d %d\n", direct, provider, observer);
    return (direct == 13 && provider == 23 && observer == 33) ? 0 : 1;
}
