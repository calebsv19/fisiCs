#line 1910 "virtual_probe_include_tokenpaste_undeclared_phase01.h"
#define JOIN_PROBE_PHASE01(a, b) a##b
#define TOKENPASTE_UNDECLARED_PROBE_PHASE01() JOIN_PROBE_PHASE01(missing_probe_token_, pasted_symbol)

static int trigger_probe_include_tokenpaste_undeclared(void) {
    return TOKENPASTE_UNDECLARED_PROBE_PHASE01();
}
