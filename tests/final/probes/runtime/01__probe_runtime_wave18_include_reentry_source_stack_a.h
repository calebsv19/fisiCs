#line 8210 "virtual_runtime_wave18_include_reentry_a_phase01.h"
#define PHASE01_W18_REENTRY_STR_INNER(x) #x
#define PHASE01_W18_REENTRY_STR(x) PHASE01_W18_REENTRY_STR_INNER(x)
enum { phase01_wave18_reentry_a_line_enum = __LINE__ };
static const char *phase01_wave18_reentry_a_text(void) {
    return "a:" __FILE__ ":" PHASE01_W18_REENTRY_STR(__LINE__);
}
#include "01__probe_runtime_wave18_include_reentry_source_stack_leaf.h"
static int phase01_wave18_reentry_a_line_value(void) {
    return phase01_wave18_reentry_a_line_enum;
}
