extern int printf(const char*, ...);

#include "01__probe_runtime_wave14_spliced_nested_macro_file_line.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave14_spliced_nested_call(),
           phase01_wave14_spliced_nested_file_text(),
           phase01_wave14_spliced_nested_line(),
           phase01_wave14_spliced_nested_file_ok());
    return 0;
}
