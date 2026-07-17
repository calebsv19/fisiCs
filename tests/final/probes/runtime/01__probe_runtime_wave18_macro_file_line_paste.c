extern int printf(const char*, ...);

#include "01__probe_runtime_wave18_macro_file_line_paste.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave18_macro_file_line_text(),
           phase01_wave18_macro_file_line_paste_text(),
           phase01_wave18_macro_file_line_value(),
           phase01_wave18_macro_file_line_file_ok());
    return 0;
}
