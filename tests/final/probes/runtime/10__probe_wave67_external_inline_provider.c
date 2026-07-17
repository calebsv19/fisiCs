#include "10__probe_wave67_external_inline_shared.h"

extern inline int wave67_external_adjust(int value);

int wave67_external_from_provider(int value) {
    return wave67_external_adjust(value);
}
