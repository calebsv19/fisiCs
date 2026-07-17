#line 7210 "virtual_runtime_wave14_inner_adjacent_phase01.h"
static const char *phase01_wave14_inner_adjacent_text(void) {
    return "inner:" __FILE__ ":tail";
}
enum { phase01_wave14_inner_adjacent_line = __LINE__ };
static int phase01_wave14_inner_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave14_inner_adjacent_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
