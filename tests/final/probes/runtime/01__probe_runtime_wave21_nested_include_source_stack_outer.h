#line 10120 "virtual_runtime_wave21_nested_outer_phase01.h"
#define PHASE01_W21_NESTED_STR_INNER(x) #x
#define PHASE01_W21_NESTED_STR(x) PHASE01_W21_NESTED_STR_INNER(x)
enum { phase01_wave21_nested_outer_entry_line = __LINE__ };
#include "01__probe_runtime_wave21_nested_include_source_stack_leaf.h"
enum { phase01_wave21_nested_outer_return_line = __LINE__ };
static const char *phase01_wave21_nested_outer_text(void) {
    return "outer:" __FILE__ ":" PHASE01_W21_NESTED_STR(phase01_wave21_nested_outer_entry_line) ":" PHASE01_W21_NESTED_STR(phase01_wave21_nested_outer_return_line);
}
static int phase01_wave21_nested_outer_file_ok(void) {
    const char *actual = __FILE__;
    const char expected[] = "virtual_runtime_wave21_nested_outer_phase01.h";
    int i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (actual[i] != expected[i]) {
            return 0;
        }
    }
    return actual[i] == '\0';
}
