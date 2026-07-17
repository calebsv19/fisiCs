extern int printf(const char*, ...);

#include "01__probe_runtime_wave17_multi_include_resumption.h"

int main(void) {
    printf("%s|%s|%s|%s|%d|%d|%d|%d\n",
           phase01_wave17_multi_outer_before_text(),
           phase01_wave17_multi_inner_text(),
           phase01_wave17_multi_leaf_text(),
           phase01_wave17_multi_outer_after_text(),
           phase01_wave17_multi_outer_before_line_value(),
           phase01_wave17_multi_inner_line_value(),
           phase01_wave17_multi_leaf_file_ok(),
           phase01_wave17_multi_outer_after_file_ok());
    return 0;
}
