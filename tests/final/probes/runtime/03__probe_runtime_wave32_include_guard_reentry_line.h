#ifndef FISICS_PROBE_RUNTIME_WAVE32_INCLUDE_GUARD_REENTRY_LINE_H
#define FISICS_PROBE_RUNTIME_WAVE32_INCLUDE_GUARD_REENTRY_LINE_H

#define W32_GUARD_CAT2(a, b) a##b
#define W32_GUARD_CAT(a, b) W32_GUARD_CAT2(a, b)
#define W32_GUARD_STR2(x) #x
#define W32_GUARD_STR(x) W32_GUARD_STR2(x)
#define W32_GUARD_DECL(name, value) enum { W32_GUARD_CAT(w32_guard_, name) = value }

#line 1200 "virtual_wave32_guard_header.h"
W32_GUARD_DECL(alpha, 41);
#define W32_GUARD_REPORT(name) \
    printf("%d %s %s %d\n", W32_GUARD_CAT(w32_guard_, name), W32_GUARD_STR(W32_GUARD_CAT(w32_guard_, name)), __FILE__, __LINE__)

#endif
