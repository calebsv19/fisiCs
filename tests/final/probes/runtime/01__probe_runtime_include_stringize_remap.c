extern int printf(const char*, ...);

#include "01__probe_runtime_include_stringize_remap.h"

int main(void) {
    printf("%s|%d|%d\n",
           phase01_runtime_stringized,
           phase01_runtime_line,
           phase01_runtime_file_ok());
    return 0;
}
