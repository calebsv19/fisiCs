#line 8310 "virtual_runtime_wave18_include_reentry_leaf_phase01.h"
static const char *phase01_wave18_reentry_leaf_text(void) {
    return "leaf:" __FILE__ ":" PHASE01_W18_REENTRY_STR(__LINE__);
}
static int phase01_wave18_reentry_leaf_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave18_include_reentry_leaf_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
