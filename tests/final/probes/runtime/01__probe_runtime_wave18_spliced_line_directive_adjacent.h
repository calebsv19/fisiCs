#define PHASE01_W18_SPLICE_FILE "virtual_runtime_wave18_spliced_line_directive_phase01.h"
#line 8110 PHASE01_W18_SPLICE_FILE
#define PHASE01_W18_SPLICE_STR_INNER(x) #x
#define PHASE01_W18_SPLICE_STR(x) PHASE01_W18_SPLICE_STR_INNER(x)
#define PHASE01_W18_SPLICE_JOIN_INNER(a, b) a##b
#define PHASE01_W18_SPLICE_JOIN(a, b) PHASE01_W18_SPLICE_JOIN_INNER(a, b)
enum { PHASE01_W18_SPLICE_JOIN(phase01_wave18_spliced_line_directive_, line_enum) = __LINE__ };
static const char *phase01_wave18_spliced_line_directive_text(void) {
    return "splice-file:" __FILE__ ":" "adjacent" \
/* phase01 wave18 keeps this splice/comment boundary runtime-neutral */ \
"-" "tail:" PHASE01_W18_SPLICE_STR(PHASE01_W18_SPLICE_JOIN(token_, source));
}
static int phase01_wave18_spliced_line_directive_line_value(void) {
    return phase01_wave18_spliced_line_directive_line_enum;
}
static int phase01_wave18_spliced_line_directive_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave18_spliced_line_directive_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
