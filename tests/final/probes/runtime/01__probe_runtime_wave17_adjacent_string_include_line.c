extern int printf(const char*, ...);

#include "01__probe_runtime_wave17_adjacent_string_include_line.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave17_adjacent_include_text(),
           phase01_wave17_adjacent_include_line_text(),
           phase01_wave17_adjacent_include_line_value(),
           phase01_wave17_adjacent_include_file_ok());
    return 0;
}
