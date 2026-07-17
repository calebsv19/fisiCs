#line 4810 "virtual_runtime_wave14_spliced_nested_phase01.h"
#define PHASE01_W14_PICK(name) name()
#define PHASE01_W14_TEXT_PAIR(left, right) left ":" right
static const char *phase01_wave14_spliced_nested_file_text(void) {
    return PHASE01_W14_TEXT_PAIR("mapped", __FILE__);
}
static int phase01_wave14_spliced_nested_line(void) { return __LINE__; }
static const char *phase01_wave14_spliced_nested_target(void) {
    return "splice-" \
/* phase01 wave14 comment between adjacent string tokens */ \
"nested";
}
static const char *phase01_wave14_spliced_nested_call(void) {
    return PHASE01_W14_PICK(phase01_wave14_spliced_\
nested_target);
}
static int phase01_wave14_spliced_nested_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave14_spliced_nested_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
