extern int printf(const char*, ...);

#include "01__probe_runtime_wave20_tokenpaste_stringize_source_provenance.h"

int main(void) {
    printf("%s|%d|%d\n",
           phase01_wave20_tokenpaste_source_text(),
           phase01_wave20_tokenpaste_value(),
           phase01_wave20_tokenpaste_file_ok());
    return 0;
}
