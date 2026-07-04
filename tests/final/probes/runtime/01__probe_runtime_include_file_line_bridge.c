extern int printf(const char*, ...);

#include "01__probe_runtime_include_file_line_bridge.h"

int main(void) {
    printf("%d\n", phase01_bridge_line_value());
    return 0;
}
