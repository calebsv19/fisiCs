extern int printf(const char*, ...);

#include "04__probe_multitu_include_param_vla_static_restrict_adjust_bridge_shared.h"

int main(void) {
    int rows = 3;
    int cols = 3;
    int matrix[3][3] = {
        {2, 4, 6},
        {1, 3, 5},
        {7, 9, 11},
    };
    printf("%d\n", wave19_multitu_include_vla_static_restrict_fold(rows, cols, matrix));
    return 0;
}
