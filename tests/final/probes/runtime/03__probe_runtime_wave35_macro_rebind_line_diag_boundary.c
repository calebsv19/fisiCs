#include <stdio.h>

#define W35_REBIND_CAT2(a, b) a##b
#define W35_REBIND_CAT(a, b) W35_REBIND_CAT2(a, b)
#define W35_REBIND_STR2(x) #x
#define W35_REBIND_STR(x) W35_REBIND_STR2(x)

#define W35_REBIND_SLOT stale
#define W35_REBIND_VALUE 100

#if 0
#error wave35 inactive diagnostic branch should not affect macro state
#undef W35_REBIND_SLOT
#define W35_REBIND_SLOT inactive
#endif

#undef W35_REBIND_SLOT
#undef W35_REBIND_VALUE
#define W35_REBIND_SLOT live
#define W35_REBIND_VALUE 44
#define W35_REBIND_DECL(slot, value) enum { W35_REBIND_CAT(w35_rt_, slot) = value }

W35_REBIND_DECL(W35_REBIND_SLOT, W35_REBIND_VALUE);

#line 3900 "virtual_wave35_rebind_main.c"
int main(void) {
    printf("%d %s %s %d\n", w35_rt_live, W35_REBIND_STR(W35_REBIND_CAT(w35_rt_, W35_REBIND_SLOT)), __FILE__, __LINE__);
    return 0;
}
