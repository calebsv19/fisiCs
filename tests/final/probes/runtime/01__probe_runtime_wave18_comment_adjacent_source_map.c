extern int printf(const char*, ...);

#include "01__probe_runtime_wave18_comment_adjacent_source_map.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave18_comment_adjacent_text(),
           phase01_wave18_comment_adjacent_line_text(),
           phase01_wave18_comment_adjacent_line_value(),
           phase01_wave18_comment_adjacent_file_ok());
    return 0;
}
