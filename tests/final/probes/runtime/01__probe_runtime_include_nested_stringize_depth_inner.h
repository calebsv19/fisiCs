#line 4810 "virtual_runtime_nested_inner_phase01.h"
#define STR_PHASE01_INNER(x) #x
#define STR_PHASE01_OUTER(x) STR_PHASE01_INNER(x)
static const char *phase01_nested_runtime_stringized = STR_PHASE01_OUTER(gamma + delta);
enum { phase01_nested_runtime_line = __LINE__ };
static int phase01_nested_runtime_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_nested_inner_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
