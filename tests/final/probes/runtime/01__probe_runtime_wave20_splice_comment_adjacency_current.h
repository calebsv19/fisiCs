#define PHASE01_W20_SPLICE_FILE "virtual_runtime_wave20_splice_current_phase01.h"
#line 9420 PHASE01_W20_SPLICE_FILE
#define PHASE01_W20_SPLICE_STR_INNER(x) #x
#define PHASE01_W20_SPLICE_STR(x) PHASE01_W20_SPLICE_STR_INNER(x)
#define PHASE01_W20_SPLICE_JOIN_INNER(a, b) a##b
#define PHASE01_W20_SPLICE_JOIN(a, b) PHASE01_W20_SPLICE_JOIN_INNER(a, b)
enum { PHASE01_W20_SPLICE_JOIN(phase01_wave20_splice_, line_value) = __LINE__ };
static const char *phase01_wave20_splice_current_text(void) {
    return "splice:" __FILE__ ":" "left" \
/* current threshold: ordinary backslash splice across comment adjacency */ \
"-" "right:" PHASE01_W20_SPLICE_STR(PHASE01_W20_SPLICE_JOIN(paste_, current));
}
static const char *phase01_wave20_splice_current_line_text(void) {
    return "line:" PHASE01_W20_SPLICE_STR(phase01_wave20_splice_line_value);
}
static int phase01_wave20_splice_current_line_value(void) {
    return phase01_wave20_splice_line_value;
}
static int phase01_wave20_splice_current_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave20_splice_current_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
