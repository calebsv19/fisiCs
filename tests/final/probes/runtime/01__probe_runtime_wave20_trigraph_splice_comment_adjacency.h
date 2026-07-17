#define PHASE01_W20_TRI_FILE "virtual_runtime_wave20_trigraph_splice_phase01.h"
#line 9410 PHASE01_W20_TRI_FILE
#define PHASE01_W20_TRI_STR_INNER(x) #x
#define PHASE01_W20_TRI_STR(x) PHASE01_W20_TRI_STR_INNER(x)
#define PHASE01_W20_TRI_JOIN_INNER(a, b) a##b
#define PHASE01_W20_TRI_JOIN(a, b) PHASE01_W20_TRI_JOIN_INNER(a, b)
enum { PHASE01_W20_TRI_JOIN(phase01_wave20_trigraph_, line_value) = __LINE__ };
static const char *phase01_wave20_trigraph_splice_text(void) {
    return "tri:" __FILE__ ":" "left" ??/
/* trigraph backslash splices through comment adjacency */ ??/
"-" "right:" PHASE01_W20_TRI_STR(PHASE01_W20_TRI_JOIN(paste_, tri));
}
static const char *phase01_wave20_trigraph_splice_line_text(void) {
    return "line:" PHASE01_W20_TRI_STR(phase01_wave20_trigraph_line_value);
}
static int phase01_wave20_trigraph_splice_line_value(void) {
    return phase01_wave20_trigraph_line_value;
}
static int phase01_wave20_trigraph_splice_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave20_trigraph_splice_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
