extern int printf(const char*, ...);

#include "01__probe_runtime_wave21_splice_comment_adjacent_source.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave21_splice_comment_text(),
           phase01_wave21_splice_comment_line_text(),
           phase01_wave21_splice_comment_runtime_line_value(),
           phase01_wave21_splice_comment_file_ok());
    return 0;
}
