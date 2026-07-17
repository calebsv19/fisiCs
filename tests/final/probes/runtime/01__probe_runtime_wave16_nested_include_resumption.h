#line 6210 "virtual_runtime_wave16_outer_phase01.h"
#define PHASE01_W16_OUTER_STR_INNER(x) #x
#define PHASE01_W16_OUTER_STR(x) PHASE01_W16_OUTER_STR_INNER(x)
enum { phase01_wave16_outer_before_line_enum = __LINE__ };
static const char *phase01_wave16_outer_before_text(void) {
    return "outer-before:" __FILE__ ":" PHASE01_W16_OUTER_STR(__LINE__);
}
#include "01__probe_runtime_wave16_nested_include_resumption_inner.h"
static const char *phase01_wave16_outer_after_text(void) {
    return "outer-after:" __FILE__ ":" PHASE01_W16_OUTER_STR(__LINE__);
}
static int phase01_wave16_outer_before_line_value(void) {
    return phase01_wave16_outer_before_line_enum;
}
static int phase01_wave16_outer_after_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave16_outer_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
