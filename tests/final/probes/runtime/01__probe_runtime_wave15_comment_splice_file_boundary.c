extern int printf(const char*, ...);

#include "01__probe_runtime_wave15_comment_splice_file_boundary.h"

int main(void) {
    printf("%s|%d|%d\n",
           phase01_wave15_comment_boundary_text(),
           phase01_wave15_comment_boundary_line_value(),
           phase01_wave15_comment_boundary_file_ok());
    return 0;
}
