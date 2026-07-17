#define PHASE01_W21_SPLICE_FILE "virtual_runtime_wave21_splice_comment_phase01.h"
#line 10210 PHASE01_W21_SPLICE_FILE
#define PHASE01_W21_SPLICE_STR_INNER(x) #x
#define PHASE01_W21_SPLICE_STR(x) PHASE01_W21_SPLICE_STR_INNER(x)
#define PHASE01_W21_SPLICE_JOIN_INNER(a, b) a##b
#define PHASE01_W21_SPLICE_JOIN(a, b) PHASE01_W21_SPLICE_JOIN_INNER(a, b)
enum { PHASE01_W21_SPLICE_JOIN(phase01_wave21_splice_comment_, line_value) = __LINE__ };
static const char *phase01_wave21_splice_comment_text(void) {
    return "splice:" __FILE__ ":" "left" \
/* ordinary backslash splice across comment replacement and adjacent strings */ \
"-" "right:" PHASE01_W21_SPLICE_STR(PHASE01_W21_SPLICE_JOIN(paste_, source));
}
static const char *phase01_wave21_splice_comment_line_text(void) {
    return "line:" PHASE01_W21_SPLICE_STR(phase01_wave21_splice_comment_line_value);
}
static int phase01_wave21_splice_comment_runtime_line_value(void) {
    return phase01_wave21_splice_comment_line_value;
}
static int phase01_wave21_splice_comment_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave21_splice_comment_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
