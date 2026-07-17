#include <stdio.h>

#define CAT2(a, b) a##b
#define CAT(a, b) CAT2(a, b)
#define STR2(x) #x
#define STR(x) STR2(x)
#define MAKE_VALUE(name, value) enum { CAT(value_, name) = value }
#define APPLY(fn, x) fn(x)

MAKE_VALUE(alpha, 19);

static int twist(int x) {
    return x * 3 + value_alpha;
}

#line 730 "virtual_wave30_macro_line_stringize_paste.c"
#define REPORT(sym) printf("%d %s %d\n", APPLY(twist, CAT(4, 2)), STR(CAT(sym, _alpha)), __LINE__)
int main(void) {
    REPORT(value);
    return 0;
}
