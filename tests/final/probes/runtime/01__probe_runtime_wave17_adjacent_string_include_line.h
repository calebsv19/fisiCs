#line 7610 "virtual_runtime_wave17_adjacent_include_phase01.h"
#define PHASE01_W17_ADJ_STR_INNER(x) #x
#define PHASE01_W17_ADJ_STR(x) PHASE01_W17_ADJ_STR_INNER(x)
enum { phase01_wave17_adjacent_include_line_enum = __LINE__ };
static const char *phase01_wave17_adjacent_include_text(void) {
    return "adjacent:" "file=" __FILE__ ":source" "-phase";
}
static const char *phase01_wave17_adjacent_include_line_text(void) {
    return "line:" PHASE01_W17_ADJ_STR(__LINE__);
}
static int phase01_wave17_adjacent_include_line_value(void) {
    return phase01_wave17_adjacent_include_line_enum;
}
static int phase01_wave17_adjacent_include_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave17_adjacent_include_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
