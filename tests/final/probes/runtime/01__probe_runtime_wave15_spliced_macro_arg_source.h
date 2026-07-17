#line 5510 "virtual_runtime_wave15_spliced_macro_phase01.h"
#define PHASE01_W15_SPLICE_PICK(name) name()
#define PHASE01_W15_SPLICE_STR_INNER(x) #x
#define PHASE01_W15_SPLICE_STR(x) PHASE01_W15_SPLICE_STR_INNER(x)
enum { phase01_wave15_spliced_macro_line_enum = __LINE__ };
static const char *phase01_wave15_spliced_macro_target(void) {
    return "spliced" \
/* phase 3 deletes this newline before macro argument tokenization */ \
"-arg";
}
static const char *phase01_wave15_spliced_macro_call(void) {
    return PHASE01_W15_SPLICE_PICK(phase01_wave15_spliced_\
macro_target);
}
static const char *phase01_wave15_spliced_macro_file_text(void) {
    return __FILE__;
}
static int phase01_wave15_spliced_macro_line_value(void) {
    return phase01_wave15_spliced_macro_line_enum;
}
static int phase01_wave15_spliced_macro_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave15_spliced_macro_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
