#include <stdio.h>

#include "01__probe_runtime_wave23_pasted_function_provenance_strict.h"

#line 23300 "virtual_wave23_pasted_function_provenance_phase01.c"
static const char *phase01_wave23_records[] = { PHASE01_W23_PROV_CAPTURE(PHASE01_W23_PROV_ROOT, __FILE__, __LINE__), PHASE01_W23_PROV_RECORD_beta(__FILE__, __LINE__) };

static int phase01_wave23_equal(const char *left, const char *right) {
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

int main(void) {
    printf("%s\n%s\n%d\n",
           phase01_wave23_records[0],
           phase01_wave23_records[1],
           phase01_wave23_equal(phase01_wave23_records[0], phase01_wave23_records[1]));
    return 0;
}
