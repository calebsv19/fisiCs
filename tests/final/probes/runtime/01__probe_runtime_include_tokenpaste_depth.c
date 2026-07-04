extern int printf(const char*, ...);

#include "01__probe_runtime_include_tokenpaste_depth.h"

int main(void) {
    printf("%d|%d\n",
           phase01_runtime_tokenpaste_value(),
           phase01_runtime_tokenpaste_file_ok());
    return 0;
}
