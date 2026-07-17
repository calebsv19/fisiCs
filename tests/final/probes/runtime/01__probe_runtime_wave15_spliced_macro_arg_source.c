extern int printf(const char*, ...);

#include "01__probe_runtime_wave15_spliced_macro_arg_source.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave15_spliced_macro_call(),
           phase01_wave15_spliced_macro_file_text(),
           phase01_wave15_spliced_macro_line_value(),
           phase01_wave15_spliced_macro_file_ok());
    return 0;
}
