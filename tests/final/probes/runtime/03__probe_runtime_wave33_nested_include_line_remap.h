#ifndef FISICS_PROBE_RUNTIME_WAVE33_NESTED_INCLUDE_LINE_REMAP_H
#define FISICS_PROBE_RUNTIME_WAVE33_NESTED_INCLUDE_LINE_REMAP_H

#include "03__probe_runtime_wave33_nested_include_line_remap_inner.h"

#define W33_NESTED_REPORT(name) \
    printf("%d %s %s %d\n", W33_INNER_CAT(w33_nested_, name), W33_INNER_STR(W33_INNER_CAT(w33_nested_, name)), __FILE__, __LINE__)

#endif
