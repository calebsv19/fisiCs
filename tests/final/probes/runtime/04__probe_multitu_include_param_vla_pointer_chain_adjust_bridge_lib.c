#include "04__probe_multitu_include_param_vla_pointer_chain_adjust_bridge_shared.h"

int wave21_multitu_vla_pointer_chain_sum(int plane_count, int cols, int (*(*planes))[cols]) {
    int total = 0;
    int i = 0;
    for (i = 0; i < plane_count; ++i) {
        total += planes[i][0][i + 1];
        total += planes[i][1][cols - 1];
    }
    return total;
}
