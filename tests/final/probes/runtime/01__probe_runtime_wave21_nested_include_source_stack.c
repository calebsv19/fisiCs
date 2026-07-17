extern int printf(const char*, ...);

#line 10100 "virtual_runtime_wave21_nested_main_phase01.c"
enum { phase01_wave21_nested_before_line = __LINE__ };
#include "01__probe_runtime_wave21_nested_include_source_stack_outer.h"
enum { phase01_wave21_nested_after_line = __LINE__ };

int main(void) {
    printf("%s|%s|%s|%d|%d|%d|%d\n",
           "main:" __FILE__,
           phase01_wave21_nested_outer_text(),
           phase01_wave21_nested_leaf_text(),
           phase01_wave21_nested_before_line,
           phase01_wave21_nested_outer_file_ok(),
           phase01_wave21_nested_leaf_file_ok(),
           phase01_wave21_nested_after_line);
    return 0;
}
