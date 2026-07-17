#ifndef FISICS_PROBE_RUNTIME_WAVE33_INCLUDE_REENTRY_MACRO_STATE_H
#define FISICS_PROBE_RUNTIME_WAVE33_INCLUDE_REENTRY_MACRO_STATE_H

#define W33_REENTRY_CAT2(a, b) a##b
#define W33_REENTRY_CAT(a, b) W33_REENTRY_CAT2(a, b)
#define W33_REENTRY_STR2(x) #x
#define W33_REENTRY_STR(x) W33_REENTRY_STR2(x)
#define W33_REENTRY_DECL(name, value) enum { W33_REENTRY_CAT(w33_reentry_, name) = value }

#line 1930 "virtual_wave33_reentry_header.h"
W33_REENTRY_DECL(beta, 91);
#define W33_REENTRY_REPORT(name, delta) \
    printf("%d %s %d %s %d\n", W33_REENTRY_CAT(w33_reentry_, name) + (delta), W33_REENTRY_STR(W33_REENTRY_CAT(w33_reentry_, name)), (delta), __FILE__, __LINE__)

#endif
