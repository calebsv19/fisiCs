extern int printf(const char*, ...);

#include "04__probe_multitu_include_param_vla_static_const_adjust_bridge_shared.h"

int main(void) {
    int rows = 3;
    int cols = 2;
    int matrix[3][2] = {
        {4, 1},
        {6, 2},
        {8, 3},
    };
    printf("%d\n", wave19_multitu_include_vla_static_const_fold(rows, cols, matrix));
    return 0;
}
