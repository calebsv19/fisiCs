extern int printf(const char*, ...);

#include "01__probe_runtime_wave16_macro_line_filename.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave16_macro_line_filename_text(),
           phase01_wave16_macro_line_filename_file_text(),
           phase01_wave16_macro_line_filename_line_value(),
           phase01_wave16_macro_line_filename_file_ok());
    return 0;
}
