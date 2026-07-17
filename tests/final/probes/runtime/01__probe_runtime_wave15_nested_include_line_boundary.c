extern int printf(const char*, ...);

#include "01__probe_runtime_wave15_nested_include_line_boundary.h"

int main(void) {
    printf("%s|%s|%d|%d|%d|%d\n",
           phase01_wave15_nested_outer_text(),
           phase01_wave15_nested_inner_text(),
           phase01_wave15_nested_outer_line_value(),
           phase01_wave15_nested_inner_line_value(),
           phase01_wave15_nested_outer_file_ok(),
           phase01_wave15_nested_inner_file_ok());
    return 0;
}
