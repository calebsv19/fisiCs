extern int printf(const char*, ...);

#include "01__probe_runtime_wave16_nested_include_resumption.h"

int main(void) {
    printf("%s|%s|%s|%d|%d|%d|%d\n",
           phase01_wave16_outer_before_text(),
           phase01_wave16_inner_text(),
           phase01_wave16_outer_after_text(),
           phase01_wave16_outer_before_line_value(),
           phase01_wave16_inner_line_value(),
           phase01_wave16_outer_after_file_ok(),
           phase01_wave16_inner_file_ok());
    return 0;
}
