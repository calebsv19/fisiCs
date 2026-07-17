extern int printf(const char*, ...);

#include "01__probe_runtime_wave20_splice_comment_adjacency_current.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave20_splice_current_text(),
           phase01_wave20_splice_current_line_text(),
           phase01_wave20_splice_current_line_value(),
           phase01_wave20_splice_current_file_ok());
    return 0;
}
