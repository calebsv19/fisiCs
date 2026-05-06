#line 810 "virtual_include_nested_macro_undeclared_phase01.h"
#define INNER_UNDECLARED_PHASE01() missing_nested_symbol
#define OUTER_UNDECLARED_PHASE01() INNER_UNDECLARED_PHASE01()

static int trigger_include_nested_macro_undeclared(void) {
    return OUTER_UNDECLARED_PHASE01();
}
