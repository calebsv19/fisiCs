extern int printf(const char*, ...);

#include "01__probe_runtime_wave18_include_reentry_source_stack_a.h"
#include "01__probe_runtime_wave18_include_reentry_source_stack_b.h"

int main(void) {
    printf("%s|%s|%s|%d|%d|%d\n",
           phase01_wave18_reentry_a_text(),
           phase01_wave18_reentry_leaf_text(),
           phase01_wave18_reentry_b_text(),
           phase01_wave18_reentry_a_line_value(),
           phase01_wave18_reentry_leaf_file_ok(),
           phase01_wave18_reentry_b_file_ok());
    return 0;
}
