extern int printf(const char*, ...);

#include "01__probe_runtime_wave17_spliced_macro_comment_source.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave17_spliced_comment_text(),
           phase01_wave17_spliced_comment_file_line_text(),
           phase01_wave17_spliced_comment_line_value(),
           phase01_wave17_spliced_comment_file_ok());
    return 0;
}
