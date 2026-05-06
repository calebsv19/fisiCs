extern int printf(const char*, ...);

#include "04__probe_multitu_include_param_vla_const_pointer_adjust_bridge_shared.h"

int main(void) {
    int rows = 3;
    int cols = 4;
    const int matrix[3][4] = {
        {3, 4, 5, 6},
        {7, 8, 9, 10},
        {11, 12, 13, 14},
    };
    printf("%d\n", wave18_multitu_include_vla_const_fold(rows, cols, matrix));
    return 0;
}
