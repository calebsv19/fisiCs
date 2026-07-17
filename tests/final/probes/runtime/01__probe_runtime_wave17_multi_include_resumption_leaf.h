#line 7310 "virtual_runtime_wave17_multi_leaf_phase01.h"
static const char *phase01_wave17_multi_leaf_text(void) {
    return "leaf:" __FILE__ ":" PHASE01_W17_MULTI_STR(__LINE__);
}
static int phase01_wave17_multi_leaf_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave17_multi_leaf_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
