extern int printf(const char*, ...);

#include "04__probe_multitu_include_param_vla_pointer_chain_adjust_bridge_shared.h"

int main(void) {
    int cols = 3;
    int alpha[2][3] = {
        {2, 4, 6},
        {8, 10, 12},
    };
    int beta[2][3] = {
        {1, 3, 5},
        {7, 9, 11},
    };
    int (*planes[2])[3] = {alpha, beta};
    printf("%d\n", wave21_multitu_vla_pointer_chain_sum(2, cols, planes));
    return 0;
}
