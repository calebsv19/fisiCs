extern int printf(const char*, ...);

#include "01__probe_runtime_wave19_splice_adjacent_source_provenance.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave19_splice_adjacent_text(),
           phase01_wave19_splice_adjacent_line_text(),
           phase01_wave19_splice_adjacent_line_value(),
           phase01_wave19_splice_adjacent_file_ok());
    return 0;
}
