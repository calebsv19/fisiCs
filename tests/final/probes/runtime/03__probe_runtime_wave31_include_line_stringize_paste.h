#ifndef FISICS_PROBE_RUNTIME_WAVE31_INCLUDE_LINE_STRINGIZE_PASTE_H
#define FISICS_PROBE_RUNTIME_WAVE31_INCLUDE_LINE_STRINGIZE_PASTE_H

#define W31_CAT2(a, b) a##b
#define W31_CAT(a, b) W31_CAT2(a, b)
#define W31_STR2(x) #x
#define W31_STR(x) W31_STR2(x)
#define W31_DECL(name, value) enum { W31_CAT(w31_value_, name) = value }

#line 880 "virtual_wave31_macro_include_header.h"
W31_DECL(beta, 23);
#define REPORT_WAVE31(sym) printf("%d %s %d\n", W31_CAT(w31_value_, sym), W31_STR(W31_CAT(w31_value_, sym)), __LINE__)

#endif
