extern int printf(const char*, ...);

#include "01__probe_runtime_wave16_splice_stringize_tokenpaste_current.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave16_splice_current_token_text(),
           phase01_wave16_splice_current_file_line_text(),
           phase01_wave16_splice_current_line_value(),
           phase01_wave16_splice_current_file_ok());
    return 0;
}
