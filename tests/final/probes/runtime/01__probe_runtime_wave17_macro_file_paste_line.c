extern int printf(const char*, ...);

#include "01__probe_runtime_wave17_macro_file_paste_line.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave17_macro_file_paste_text(),
           phase01_wave17_macro_file_paste_adjacent_text(),
           phase01_wave17_macro_file_paste_line_value(),
           phase01_wave17_macro_file_paste_file_ok());
    return 0;
}
