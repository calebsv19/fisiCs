#ifndef FISICS_PROBE_RUNTIME_WAVE34_NESTED_INCLUDE_LINE_VALUE_H
#define FISICS_PROBE_RUNTIME_WAVE34_NESTED_INCLUDE_LINE_VALUE_H

#include "03__probe_runtime_wave34_nested_include_line_value_inner.h"

#define W34_NESTED_REPORT(name) \
    printf("%d %s %s %d\n", W34_NESTED_CAT(w34_nested_, name), W34_NESTED_STR(W34_NESTED_CAT(w34_nested_, name)), __FILE__, __LINE__)

#endif
