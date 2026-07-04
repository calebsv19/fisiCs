#line 2710 "virtual_runtime_nested_outer_phase01.h"
#include "01__probe_runtime_include_nested_stringize_depth_inner.h"
static int phase01_nested_outer_line(void) { return __LINE__; }
static int phase01_nested_outer_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_nested_outer_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
