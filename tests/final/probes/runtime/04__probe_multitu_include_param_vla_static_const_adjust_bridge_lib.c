#include "04__probe_multitu_include_param_vla_static_const_adjust_bridge_shared.h"

int wave19_multitu_include_vla_static_const_fold(int rows, int cols, int (*matrix)[cols]) {
    return matrix[0][0] * matrix[1][0] + matrix[2][1] + rows + cols;
}
