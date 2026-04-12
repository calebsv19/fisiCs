#include "10__line_directive_include_extern_array_def_mismatch_diag_current_nodiag.h"

int arr_def_inc_diag[4] = {1, 2, 3, 4};

int main(void) {
    return arr_def_inc_diag[0];
}
