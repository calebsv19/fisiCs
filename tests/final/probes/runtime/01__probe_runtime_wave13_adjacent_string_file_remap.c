extern int printf(const char*, ...);

#include "01__probe_runtime_wave13_adjacent_string_file_remap.h"

int main(void) {
    printf("%s|%d|%d\n",
           phase01_wave13_adjacent_text(),
           phase01_wave13_adjacent_line,
           phase01_wave13_adjacent_file_ok());
    return 0;
}
