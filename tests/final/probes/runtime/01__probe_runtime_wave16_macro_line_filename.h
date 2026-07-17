#define PHASE01_W16_MACRO_FILENAME "virtual_runtime_wave16_macro_line_filename_phase01.h"
#line 6410 PHASE01_W16_MACRO_FILENAME
#define PHASE01_W16_MACRO_FILE_STR_INNER(x) #x
#define PHASE01_W16_MACRO_FILE_STR(x) PHASE01_W16_MACRO_FILE_STR_INNER(x)
enum { phase01_wave16_macro_line_filename_line_enum = __LINE__ };
static const char *phase01_wave16_macro_line_filename_text(void) {
    return "macro-file:" __FILE__ ":" PHASE01_W16_MACRO_FILE_STR(__LINE__);
}
static const char *phase01_wave16_macro_line_filename_file_text(void) {
    return "adjacent:" "__FILE__=" __FILE__;
}
static int phase01_wave16_macro_line_filename_line_value(void) {
    return phase01_wave16_macro_line_filename_line_enum;
}
static int phase01_wave16_macro_line_filename_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave16_macro_line_filename_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
