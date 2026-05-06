#line 2410 "virtual_include_nested_stringize_phase01.h"
#define STR_PHASE01_INNER(x) #x
#define STR_PHASE01_OUTER(x) STR_PHASE01_INNER(x)

static const char *phase01_nested_stringized =
    STR_PHASE01_OUTER(gamma + delta);

int main(void) {
    return phase01_nested_stringized[0] == 'g' ? 0 : 1;
}
