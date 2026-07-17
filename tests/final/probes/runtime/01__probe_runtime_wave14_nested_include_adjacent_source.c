extern int printf(const char*, ...);

#include "01__probe_runtime_wave14_nested_include_adjacent_source.h"

int main(void) {
    printf("%s|%s|%d|%d|%d|%d\n",
           phase01_wave14_outer_adjacent_text(),
           phase01_wave14_inner_adjacent_text(),
           phase01_wave14_outer_adjacent_line,
           phase01_wave14_inner_adjacent_line,
           phase01_wave14_outer_file_ok(),
           phase01_wave14_inner_file_ok());
    return 0;
}
