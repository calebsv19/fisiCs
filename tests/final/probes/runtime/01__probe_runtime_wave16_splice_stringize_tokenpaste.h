#line 6110 "virtual_runtime_wave16_splice_token_phase01.h"
#define PHASE01_W16_SPLICE_STR_INNER(x) #x
#define PHASE01_W16_SPLICE_STR(x) PHASE01_W16_SPLICE_STR_INNER(x)
#define PHASE01_W16_SPLICE_CAT_INNER(a, b) a##b
#define PHASE01_W16_SPLICE_CAT(a, b) PHASE01_W16_SPLICE_CAT_INNER(a, b)
enum { PHASE01_W16_SPLICE_CAT(phase01_wave16_splice_, line_enum) = __LINE__ };
static const char *phase01_wave16_splice_token_text(void) {
    return PHASE01_W16_SPLICE_STR(PHASE01_W16_SPLICE_CAT(source_, token));
}
static const char *phase01_wave16_splice_file_line_text(void) {
    return "file:" __FILE__ \
/* phase 3 splices this adjacent string boundary before comments vanish */ \
"|line:" PHASE01_W16_SPLICE_STR(__LINE__);
}
static int phase01_wave16_splice_line_value(void) {
    return phase01_wave16_splice_line_enum;
}
static int phase01_wave16_splice_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave16_splice_token_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
