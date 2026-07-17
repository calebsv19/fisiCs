#define PHASE01_W17_MACRO_FILE "virtual_runtime_wave17_macro_file_paste_phase01.h"
#line 7510 PHASE01_W17_MACRO_FILE
#define PHASE01_W17_FILE_STR_INNER(x) #x
#define PHASE01_W17_FILE_STR(x) PHASE01_W17_FILE_STR_INNER(x)
#define PHASE01_W17_FILE_CAT_INNER(a, b) a##b
#define PHASE01_W17_FILE_CAT(a, b) PHASE01_W17_FILE_CAT_INNER(a, b)
enum { PHASE01_W17_FILE_CAT(phase01_wave17_macro_file_paste_, line_enum) = __LINE__ };
static const char *phase01_wave17_macro_file_paste_text(void) {
    return "macro-file:" __FILE__ ":" PHASE01_W17_FILE_STR(__LINE__);
}
static const char *phase01_wave17_macro_file_paste_adjacent_text(void) {
    return "token:" PHASE01_W17_FILE_STR(PHASE01_W17_FILE_CAT(paste_, value)) ":" "__FILE__=" __FILE__;
}
static int phase01_wave17_macro_file_paste_line_value(void) {
    return phase01_wave17_macro_file_paste_line_enum;
}
static int phase01_wave17_macro_file_paste_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave17_macro_file_paste_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
