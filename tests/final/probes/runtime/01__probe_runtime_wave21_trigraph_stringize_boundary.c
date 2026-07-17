extern int printf(const char*, ...);

#include "01__probe_runtime_wave21_trigraph_stringize_boundary.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave21_trigraph_boundary_text(),
           phase01_wave21_trigraph_boundary_line_text(),
           phase01_wave21_trigraph_boundary_line_value(),
           phase01_wave21_trigraph_boundary_file_ok());
    return 0;
}
