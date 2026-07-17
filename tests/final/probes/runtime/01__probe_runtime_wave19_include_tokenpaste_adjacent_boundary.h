#line 9310 "virtual_runtime_wave19_tokenpaste_adjacent_phase01.h"
#define PHASE01_W19_TOK_STR_INNER(x) #x
#define PHASE01_W19_TOK_STR(x) PHASE01_W19_TOK_STR_INNER(x)
#define PHASE01_W19_TOK_JOIN_INNER(a, b) a##b
#define PHASE01_W19_TOK_JOIN(a, b) PHASE01_W19_TOK_JOIN_INNER(a, b)
enum { PHASE01_W19_TOK_JOIN(phase01_wave19_tokenpaste_, line_value) = __LINE__ };
static const char *phase01_wave19_tokenpaste_adjacent_text(void) {
    return "token:" __FILE__ ":" "adjacent" "-" PHASE01_W19_TOK_STR(PHASE01_W19_TOK_JOIN(paste_, source));
}
static const char *phase01_wave19_tokenpaste_adjacent_line_text(void) {
    return "line:" PHASE01_W19_TOK_STR(phase01_wave19_tokenpaste_line_value);
}
static int phase01_wave19_tokenpaste_adjacent_value(void) {
    return phase01_wave19_tokenpaste_line_value;
}
static int phase01_wave19_tokenpaste_adjacent_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave19_tokenpaste_adjacent_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
