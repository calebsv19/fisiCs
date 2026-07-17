#include <stdio.h>

#define PHASE01_W23_FN_CAT_RAW(a, b) a##b
#define PHASE01_W23_FN_CAT(a, b) PHASE01_W23_FN_CAT_RAW(a, b)
#define PHASE01_W23_FN_ROOT alpha
#define PHASE01_W23_FN_alpha(x) ((x) * 3 + 1)
#define PHASE01_W23_FN_CALL_RAW(name, value) PHASE01_W23_FN_CAT(PHASE01_W23_FN_, name)(value)
#define PHASE01_W23_FN_CALL(name, value) PHASE01_W23_FN_CALL_RAW(name, value)

#line 23200 "virtual_wave23_pasted_function_rescan_phase01.c"
enum { phase01_wave23_function_value = PHASE01_W23_FN_CALL(PHASE01_W23_FN_ROOT, 7) };

int main(void) {
    printf("%d %s %d\n", phase01_wave23_function_value, __FILE__, __LINE__);
    return 0;
}
