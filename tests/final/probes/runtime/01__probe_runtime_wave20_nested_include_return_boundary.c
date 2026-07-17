extern int printf(const char*, ...);

#line 9500 "virtual_runtime_wave20_nested_main_phase01.c"
enum { phase01_wave20_nested_before_line = __LINE__ };
#include "01__probe_runtime_wave20_nested_include_return_outer.h"
enum { phase01_wave20_nested_after_line = __LINE__ };

int main(void) {
    printf("%s|%s|%s|%d|%d|%d|%d\n",
           "before:" __FILE__,
           phase01_wave20_nested_outer_text(),
           phase01_wave20_nested_inner_text(),
           phase01_wave20_nested_before_line,
           phase01_wave20_nested_outer_file_ok(),
           phase01_wave20_nested_inner_file_ok(),
           phase01_wave20_nested_after_line);
    return 0;
}
