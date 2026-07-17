#line 5410 "virtual_runtime_wave15_nested_inner_phase01.h"
#define PHASE01_W15_NESTED_INNER_STR_INNER(x) #x
#define PHASE01_W15_NESTED_INNER_STR(x) PHASE01_W15_NESTED_INNER_STR_INNER(x)
enum { phase01_wave15_nested_inner_line_enum = __LINE__ };
static const char *phase01_wave15_nested_inner_text(void) {
    return "inner:" __FILE__ ":" PHASE01_W15_NESTED_INNER_STR(__LINE__);
}
static int phase01_wave15_nested_inner_line_value(void) {
    return phase01_wave15_nested_inner_line_enum;
}
static int phase01_wave15_nested_inner_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave15_nested_inner_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
