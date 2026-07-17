#line 4510 "virtual_runtime_wave13_spliced_macro_phase01.h"
#define PHASE01_W13_PICK(name) name()
static const char *phase01_wave13_spliced_text(void) { return __FILE__; }
static int phase01_wave13_spliced_line(void) { return __LINE__; }
static const char *phase01_wave13_spliced_comment_text(void) {
    return "splice-" \
/* phase01 comment between adjacent string tokens */ \
"comment";
}
static const char *phase01_wave13_spliced_call(void) {
    return PHASE01_W13_PICK(phase01_wave13_spliced_\
text);
}
static int phase01_wave13_spliced_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave13_spliced_macro_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
