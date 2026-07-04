#line 3810 "virtual_runtime_include_tokenpaste_phase01.h"
#define JOIN_PHASE01(a, b) a##b
#define MAKE_PHASE01_NAME(a, b) JOIN_PHASE01(a, b)
#define DECL_PHASE01_VALUE(tag) enum { MAKE_PHASE01_NAME(tag, _mapped_line) = __LINE__ }
DECL_PHASE01_VALUE(phase01_tokenpaste);
static int phase01_runtime_tokenpaste_value(void) { return phase01_tokenpaste_mapped_line; }
static int phase01_runtime_tokenpaste_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_include_tokenpaste_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
