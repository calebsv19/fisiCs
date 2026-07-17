#line 9110 "virtual_runtime_wave19_resumption_header_phase01.h"
enum { phase01_wave19_resumption_header_line = __LINE__ };
static const char *phase01_wave19_resumption_header_text(void) {
    return "header:" __FILE__ ":" PHASE01_W19_RESUME_STR(phase01_wave19_resumption_header_line);
}
static int phase01_wave19_resumption_header_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave19_resumption_header_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
