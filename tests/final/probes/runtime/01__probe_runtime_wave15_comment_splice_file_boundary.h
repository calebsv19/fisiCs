#line 5210 "virtual_runtime_wave15_comment_splice_phase01.h"
#define PHASE01_W15_COMMENT_STR_INNER(x) #x
#define PHASE01_W15_COMMENT_STR(x) PHASE01_W15_COMMENT_STR_INNER(x)
#define PHASE01_W15_COMMENT_CAT_INNER(a, b) a##b
#define PHASE01_W15_COMMENT_CAT(a, b) PHASE01_W15_COMMENT_CAT_INNER(a, b)
enum { PHASE01_W15_COMMENT_CAT(phase01_wave15_comment_, line_enum) = __LINE__ };
static const char *phase01_wave15_comment_boundary_text(void) {
    return "source" /**/ "-boundary" \
"|" __FILE__;
}
static const char *phase01_wave15_comment_boundary_line_text(void) {
    return PHASE01_W15_COMMENT_STR(__LINE__);
}
static int phase01_wave15_comment_boundary_line_value(void) {
    return phase01_wave15_comment_line_enum;
}
static int phase01_wave15_comment_boundary_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave15_comment_splice_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
