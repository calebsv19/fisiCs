#line 2610 "virtual_runtime_include_stringize_phase01.h"
#define STR_PHASE01(x) #x
static const char *phase01_runtime_stringized = STR_PHASE01(alpha + beta);
enum { phase01_runtime_line = __LINE__ };
static int phase01_runtime_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_include_stringize_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
