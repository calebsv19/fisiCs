#line 2210 "virtual_include_stringize_phase01.h"
#define STR_PHASE01(x) #x

static const char *phase01_stringized = STR_PHASE01(alpha + beta);

int main(void) {
    return phase01_stringized[0] == 'a' ? 0 : 1;
}
