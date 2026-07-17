extern int printf(const char*, ...);

#include "01__probe_runtime_wave13_tokenpaste_line_stringize.h"

int main(void) {
    printf("%s|%d|%d\n",
           phase01_wave13_tokenpaste_line_text(),
           phase01_wave13_tokenpaste_line_value(),
           phase01_wave13_tokenpaste_file_ok());
    return 0;
}
