extern int printf(const char*, ...);

#include "04__probe_multitu_include_param_vla_restrict_pointer_adjust_bridge_shared.h"

int main(void) {
    int rows = 2;
    int cols = 4;
    int matrix[2][4] = {
        {1, 2, 3, 4},
        {7, 8, 9, 10},
    };
    printf("%d\n", wave18_multitu_include_vla_restrict_fold(rows, cols, matrix));
    return 0;
}
