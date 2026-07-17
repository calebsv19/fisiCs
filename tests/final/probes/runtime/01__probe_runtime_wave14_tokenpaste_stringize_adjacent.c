extern int printf(const char*, ...);

#include "01__probe_runtime_wave14_tokenpaste_stringize_adjacent.h"

int main(void) {
    printf("%s|%d|%s|%d\n",
           phase01_wave14_tokenpaste_line_text(),
           phase01_wave14_tokenpaste_line_value(),
           phase01_wave14_tokenpaste_file_text(),
           phase01_wave14_tokenpaste_file_ok());
    return 0;
}
