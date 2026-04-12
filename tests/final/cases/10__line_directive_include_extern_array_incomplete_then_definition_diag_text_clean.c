#include "10__line_directive_include_extern_array_incomplete_then_definition_diag_text_clean.h"

int arr_inc_include_clean[4] = {1, 2, 3, 4};

int main(void) {
    return arr_inc_include_clean[3];
}
