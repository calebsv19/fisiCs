#line 1810 "virtual_probe_include_nested_macro_undeclared_phase01.h"
#define INNER_UNDECLARED_PROBE_PHASE01() missing_nested_probe_symbol
#define OUTER_UNDECLARED_PROBE_PHASE01() INNER_UNDECLARED_PROBE_PHASE01()

static int trigger_probe_include_nested_macro_undeclared(void) {
    return OUTER_UNDECLARED_PROBE_PHASE01();
}
