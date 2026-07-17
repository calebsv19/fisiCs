extern int printf(const char*, ...);

#include "01__probe_runtime_wave21_tokenpaste_stringize_line_provenance.h"

int main(void) {
    printf("%s|%d|%d\n",
           phase01_wave21_tokenpaste_source_text(),
           phase01_wave21_tokenpaste_runtime_line_value(),
           phase01_wave21_tokenpaste_file_ok());
    return 0;
}
