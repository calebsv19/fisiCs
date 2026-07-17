#line 9610 "virtual_runtime_wave20_nested_inner_phase01.h"
enum { phase01_wave20_nested_inner_line = __LINE__ };
static const char *phase01_wave20_nested_inner_text(void) {
    return "inner:" __FILE__ ":" PHASE01_W20_NESTED_OUTER_STR(phase01_wave20_nested_inner_line);
}
static int phase01_wave20_nested_inner_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave20_nested_inner_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
