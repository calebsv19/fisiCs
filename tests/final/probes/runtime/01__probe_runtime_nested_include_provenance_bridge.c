extern int printf(const char*, ...);

#include "01__probe_runtime_nested_include_provenance_bridge.h"

int main(void) {
    printf("%d|%d\n",
           phase01_nested_provenance_inner_line(),
           phase01_nested_provenance_outer_line());
    return 0;
}
