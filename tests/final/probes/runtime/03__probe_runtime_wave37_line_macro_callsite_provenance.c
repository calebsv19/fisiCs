#include <stdio.h>

#include "03__probe_runtime_wave37_line_macro_callsite_provenance.h"

#line 5400 "virtual_wave37_line_macro_main.c"
int main(void) {
    printf("%d %s %d %s %s %d\n", W37_LINE_CAPTURE(), W37_LINE_FILE(), W37_LINE_VALUE(8), W37_LINE_STR(W37_LINE_CAT(w37_line_, marker)), __FILE__, __LINE__);
    return 0;
}
