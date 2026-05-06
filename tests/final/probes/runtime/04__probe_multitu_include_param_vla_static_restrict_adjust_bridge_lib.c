#include "04__probe_multitu_include_param_vla_static_restrict_adjust_bridge_shared.h"

int wave19_multitu_include_vla_static_restrict_fold(int rows, int cols, int (*restrict matrix)[cols]) {
    return matrix[0][2] + matrix[1][1] * matrix[2][0] + rows - cols;
}
