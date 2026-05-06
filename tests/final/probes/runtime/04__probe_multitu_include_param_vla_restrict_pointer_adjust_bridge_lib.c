#include "04__probe_multitu_include_param_vla_restrict_pointer_adjust_bridge_shared.h"

int wave18_multitu_include_vla_restrict_fold(int rows, int cols, int (*restrict matrix)[cols]) {
    return matrix[0][2] * matrix[1][3] + rows + cols;
}
