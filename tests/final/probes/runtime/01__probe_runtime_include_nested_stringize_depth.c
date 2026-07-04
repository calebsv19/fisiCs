extern int printf(const char*, ...);

#include "01__probe_runtime_include_nested_stringize_depth.h"

int main(void) {
    printf("%s|%d|%d|%d|%d\n",
           phase01_nested_runtime_stringized,
           phase01_nested_runtime_line,
           phase01_nested_runtime_file_ok(),
           phase01_nested_outer_line(),
           phase01_nested_outer_file_ok());
    return 0;
}
