#ifndef FISICS_PROBE_RUNTIME_WAVE34_NESTED_INCLUDE_LINE_VALUE_INNER_H
#define FISICS_PROBE_RUNTIME_WAVE34_NESTED_INCLUDE_LINE_VALUE_INNER_H

#define W34_NESTED_CAT2(a, b) a##b
#define W34_NESTED_CAT(a, b) W34_NESTED_CAT2(a, b)
#define W34_NESTED_STR2(x) #x
#define W34_NESTED_STR(x) W34_NESTED_STR2(x)
#define W34_NESTED_DECL(name, base, line) enum { W34_NESTED_CAT(w34_nested_, name) = (base) + (line) }

#line 2700 "virtual_wave34_nested_inner.h"
W34_NESTED_DECL(delta, 3, __LINE__);

#endif
