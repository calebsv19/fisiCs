#ifndef FISICS_PROBE_RUNTIME_WAVE33_NESTED_INCLUDE_LINE_REMAP_INNER_H
#define FISICS_PROBE_RUNTIME_WAVE33_NESTED_INCLUDE_LINE_REMAP_INNER_H

#define W33_INNER_CAT2(a, b) a##b
#define W33_INNER_CAT(a, b) W33_INNER_CAT2(a, b)
#define W33_INNER_STR2(x) #x
#define W33_INNER_STR(x) W33_INNER_STR2(x)
#define W33_INNER_DECL(name, value) enum { W33_INNER_CAT(w33_nested_, name) = value }

#line 2070 "virtual_wave33_nested_inner.h"
W33_INNER_DECL(gamma, 58);

#endif
