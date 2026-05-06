#line 910 "virtual_include_tokenpaste_undeclared_phase01.h"
#define JOIN_PHASE01(a, b) a##b
#define TOKENPASTE_UNDECLARED_PHASE01() JOIN_PHASE01(missing_token_, pasted_symbol)

static int trigger_include_tokenpaste_undeclared(void) {
    return TOKENPASTE_UNDECLARED_PHASE01();
}
