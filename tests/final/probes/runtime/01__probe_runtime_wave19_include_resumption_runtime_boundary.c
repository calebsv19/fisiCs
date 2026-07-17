extern int printf(const char*, ...);

#define PHASE01_W19_RESUME_STR_INNER(x) #x
#define PHASE01_W19_RESUME_STR(x) PHASE01_W19_RESUME_STR_INNER(x)

#line 9010 "virtual_runtime_wave19_resumption_main_phase01.c"
enum { phase01_wave19_resumption_before_line = __LINE__ };
#include "01__probe_runtime_wave19_include_resumption_runtime_boundary.h"
enum { phase01_wave19_resumption_after_line = __LINE__ };

int main(void) {
    printf("%s|%s|%s|%d|%d|%d\n",
           "before:" __FILE__ ":" PHASE01_W19_RESUME_STR(phase01_wave19_resumption_before_line),
           phase01_wave19_resumption_header_text(),
           "after:" __FILE__ ":" PHASE01_W19_RESUME_STR(phase01_wave19_resumption_after_line),
           phase01_wave19_resumption_before_line,
           phase01_wave19_resumption_header_file_ok(),
           phase01_wave19_resumption_after_line);
    return 0;
}
