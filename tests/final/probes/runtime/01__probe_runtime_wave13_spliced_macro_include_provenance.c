extern int printf(const char*, ...);

#include "01__probe_runtime_wave13_spliced_macro_include_provenance.h"

int main(void) {
    printf("%s|%s|%d|%d\n",
           phase01_wave13_spliced_call(),
           phase01_wave13_spliced_comment_text(),
           phase01_wave13_spliced_line(),
           phase01_wave13_spliced_file_ok());
    return 0;
}
