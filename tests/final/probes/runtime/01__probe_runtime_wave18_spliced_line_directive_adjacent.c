extern int printf(const char*, ...);

#include "01__probe_runtime_wave18_spliced_line_directive_adjacent.h"

int main(void) {
    printf("%s|%d|%d\n",
           phase01_wave18_spliced_line_directive_text(),
           phase01_wave18_spliced_line_directive_line_value(),
           phase01_wave18_spliced_line_directive_file_ok());
    return 0;
}
