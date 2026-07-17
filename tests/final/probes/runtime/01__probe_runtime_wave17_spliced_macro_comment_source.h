#line 7410 "virtual_runtime_wave17_spliced_comment_phase01.h"
#define PHASE01_W17_SPLICE_STR_INNER(x) #x
#define PHASE01_W17_SPLICE_STR(x) PHASE01_W17_SPLICE_STR_INNER(x)
#define PHASE01_W17_SPLICE_JOIN_INNER(a, b) a##b
#define PHASE01_W17_SPLICE_JOIN(a, b) PHASE01_W17_SPLICE_JOIN_INNER(a, b)
enum { PHASE01_W17_SPLICE_JOIN(phase01_wave17_spliced_comment_, line_enum) = __LINE__ };
static const char *phase01_wave17_spliced_comment_text(void) {
    return "comment" /**/ "-splice:" PHASE01_W17_SPLICE_STR(PHASE01_W17_SPLICE_JOIN(source_, token));
}
static const char *phase01_wave17_spliced_comment_file_line_text(void) {
    return "file:" __FILE__ \
/* phase01 wave17 splices this line before comment replacement */ \
":line:" PHASE01_W17_SPLICE_STR(__LINE__);
}
static int phase01_wave17_spliced_comment_line_value(void) {
    return phase01_wave17_spliced_comment_line_enum;
}
static int phase01_wave17_spliced_comment_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave17_spliced_comment_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
