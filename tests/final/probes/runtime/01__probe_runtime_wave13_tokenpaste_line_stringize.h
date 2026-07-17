#line 4410 "virtual_runtime_wave13_tokenpaste_line_phase01.h"
#define PHASE01_W13_CAT_INNER(a, b) a##b
#define PHASE01_W13_CAT(a, b) PHASE01_W13_CAT_INNER(a, b)
#define PHASE01_W13_STR_INNER(x) #x
#define PHASE01_W13_STR(x) PHASE01_W13_STR_INNER(x)
enum { PHASE01_W13_CAT(phase01_wave13_tokenpaste_, line_enum) = __LINE__ };
static const char *phase01_wave13_tokenpaste_line_text(void) { return PHASE01_W13_STR(__LINE__); }
static int phase01_wave13_tokenpaste_line_value(void) { return phase01_wave13_tokenpaste_line_enum; }
static int phase01_wave13_tokenpaste_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave13_tokenpaste_line_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
