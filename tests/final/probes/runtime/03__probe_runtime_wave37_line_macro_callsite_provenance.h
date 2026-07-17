#ifndef FISICS_PROBE_RUNTIME_WAVE37_LINE_MACRO_CALLSITE_PROVENANCE_H
#define FISICS_PROBE_RUNTIME_WAVE37_LINE_MACRO_CALLSITE_PROVENANCE_H

#define W37_LINE_CAT2(a, b) a##b
#define W37_LINE_CAT(a, b) W37_LINE_CAT2(a, b)
#define W37_LINE_STR2(x) #x
#define W37_LINE_STR(x) W37_LINE_STR2(x)
#define W37_LINE_CAPTURE() __LINE__
#define W37_LINE_FILE() __FILE__
#define W37_LINE_VALUE(x) ((x) + W37_LINE_BONUS)

#endif

#ifndef W37_LINE_BONUS
#define W37_LINE_BONUS 5
#endif

#line 5380 "virtual_wave37_line_macro_header.h"
enum { w37_line_marker = W37_LINE_VALUE(10) };
