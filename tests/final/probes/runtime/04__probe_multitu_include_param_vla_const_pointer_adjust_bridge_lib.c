#include "04__probe_multitu_include_param_vla_const_pointer_adjust_bridge_shared.h"

int wave18_multitu_include_vla_const_fold(int rows, int cols, const int (*matrix)[cols]) {
    return matrix[0][3] + matrix[1][1] + matrix[2][2] + cols;
}
