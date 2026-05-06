#include "04__probe_multitu_include_param_array_restrict_adjust_bridge_shared.h"

int wave17_multitu_include_restrict_fold(int *restrict values) {
    return values[0] * values[1] + values[2] + values[3] + 8;
}
