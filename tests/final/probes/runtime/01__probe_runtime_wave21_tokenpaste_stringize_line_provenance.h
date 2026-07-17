#define PHASE01_W21_TOK_FILE "virtual_runtime_wave21_tokenpaste_phase01.h"
#line 10330 PHASE01_W21_TOK_FILE
#define PHASE01_W21_TOK_STR_INNER(x) #x
#define PHASE01_W21_TOK_STR(x) PHASE01_W21_TOK_STR_INNER(x)
#define PHASE01_W21_TOK_JOIN_INNER(a, b) a##b
#define PHASE01_W21_TOK_JOIN(a, b) PHASE01_W21_TOK_JOIN_INNER(a, b)
enum { PHASE01_W21_TOK_JOIN(phase01_wave21_tokenpaste_, line_value) = __LINE__ };
static const char *phase01_wave21_tokenpaste_source_text(void) {
    return "tok:" __FILE__ ":" PHASE01_W21_TOK_STR(PHASE01_W21_TOK_JOIN(phase01_wave21_tokenpaste_, line_value)) ":" PHASE01_W21_TOK_STR(PHASE01_W21_TOK_JOIN(call_, provenance));
}
static int phase01_wave21_tokenpaste_runtime_line_value(void) {
    return phase01_wave21_tokenpaste_line_value;
}
static int phase01_wave21_tokenpaste_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave21_tokenpaste_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
