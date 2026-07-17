#define PHASE01_W19_SPLICE_FILE "virtual_runtime_wave19_splice_adjacent_phase01.h"
#line 9210 PHASE01_W19_SPLICE_FILE
#define PHASE01_W19_SPLICE_STR_INNER(x) #x
#define PHASE01_W19_SPLICE_STR(x) PHASE01_W19_SPLICE_STR_INNER(x)
#define PHASE01_W19_SPLICE_JOIN_INNER(a, b) a##b
#define PHASE01_W19_SPLICE_JOIN(a, b) PHASE01_W19_SPLICE_JOIN_INNER(a, b)
enum { PHASE01_W19_SPLICE_JOIN(phase01_wave19_splice_adjacent_, line_enum) = __LINE__ };
static const char *phase01_wave19_splice_adjacent_text(void) {
    return "splice:" __FILE__ ":" "head" \
/* wave19 stresses comment deletion at a splice/adjacent literal boundary */ \
"-" "tail:" PHASE01_W19_SPLICE_STR(PHASE01_W19_SPLICE_JOIN(token_, boundary));
}
static const char *phase01_wave19_splice_adjacent_line_text(void) {
    return "line:" PHASE01_W19_SPLICE_STR(phase01_wave19_splice_adjacent_line_enum);
}
static int phase01_wave19_splice_adjacent_line_value(void) {
    return phase01_wave19_splice_adjacent_line_enum;
}
static int phase01_wave19_splice_adjacent_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave19_splice_adjacent_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
