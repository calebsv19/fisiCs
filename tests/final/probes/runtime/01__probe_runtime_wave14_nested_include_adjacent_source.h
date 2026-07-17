#line 4610 "virtual_runtime_wave14_outer_adjacent_phase01.h"
#include "01__probe_runtime_wave14_nested_include_adjacent_source_inner.h"
static const char *phase01_wave14_outer_adjacent_text(void) {
    return "outer:" __FILE__ ":tail";
}
enum { phase01_wave14_outer_adjacent_line = __LINE__ };
static int phase01_wave14_outer_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave14_outer_adjacent_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
