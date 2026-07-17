#define PHASE01_W18_FILE_NAME "virtual_runtime_wave18_macro_file_line_phase01.h"
#line 8510 PHASE01_W18_FILE_NAME
#define PHASE01_W18_FILE_STR_INNER(x) #x
#define PHASE01_W18_FILE_STR(x) PHASE01_W18_FILE_STR_INNER(x)
#define PHASE01_W18_FILE_CAT_INNER(a, b) a##b
#define PHASE01_W18_FILE_CAT(a, b) PHASE01_W18_FILE_CAT_INNER(a, b)
enum { PHASE01_W18_FILE_CAT(phase01_wave18_macro_file_line_, enum_value) = __LINE__ };
static const char *phase01_wave18_macro_file_line_text(void) {
    return "macro-file:" __FILE__ ":line:" PHASE01_W18_FILE_STR(__LINE__);
}
static const char *phase01_wave18_macro_file_line_paste_text(void) {
    return "paste:" PHASE01_W18_FILE_STR(PHASE01_W18_FILE_CAT(source_, marker)) ":" "__FILE__=" __FILE__;
}
static int phase01_wave18_macro_file_line_value(void) {
    return phase01_wave18_macro_file_line_enum_value;
}
static int phase01_wave18_macro_file_line_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave18_macro_file_line_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
