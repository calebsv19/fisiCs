#line 10140 "virtual_runtime_wave21_nested_leaf_phase01.h"
enum { phase01_wave21_nested_leaf_line = __LINE__ };
static const char *phase01_wave21_nested_leaf_text(void) {
    return "leaf:" __FILE__ ":" PHASE01_W21_NESTED_STR(phase01_wave21_nested_leaf_line);
}
static int phase01_wave21_nested_leaf_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave21_nested_leaf_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
