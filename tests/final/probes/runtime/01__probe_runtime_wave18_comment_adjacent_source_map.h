#line 8610 "virtual_runtime_wave18_comment_adjacent_phase01.h"
#define PHASE01_W18_COMMENT_STR_INNER(x) #x
#define PHASE01_W18_COMMENT_STR(x) PHASE01_W18_COMMENT_STR_INNER(x)
enum { phase01_wave18_comment_adjacent_line_enum = __LINE__ };
static const char *phase01_wave18_comment_adjacent_text(void) {
    return "comment" /**/ "-adjacent:" "__FILE__=" __FILE__;
}
static const char *phase01_wave18_comment_adjacent_line_text(void) {
    return "line:" PHASE01_W18_COMMENT_STR(__LINE__) ":" "source" /**/ "-map";
}
static int phase01_wave18_comment_adjacent_line_value(void) {
    return phase01_wave18_comment_adjacent_line_enum;
}
static int phase01_wave18_comment_adjacent_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave18_comment_adjacent_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
