extern int printf(const char*, ...);

#include "01__probe_runtime_include_tokenpaste_stringize_bridge.h"

int main(void) {
    printf("%s|%d\n",
           phase01_bridge_text(),
           phase01_bridge_line_value());
    return 0;
}
