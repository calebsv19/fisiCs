extern int printf(const char*, ...);

#include "01__probe_runtime_wave20_trigraph_splice_comment_adjacency.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave20_trigraph_splice_text(),
           phase01_wave20_trigraph_splice_line_text(),
           phase01_wave20_trigraph_splice_line_value(),
           phase01_wave20_trigraph_splice_file_ok());
    return 0;
}
