#include <stdio.h>

#define W34_IB_CAT2(a, b) a##b
#define W34_IB_CAT(a, b) W34_IB_CAT2(a, b)
#define W34_IB_STR2(x) #x
#define W34_IB_STR(x) W34_IB_STR2(x)
#define W34_IB_PICK active_token
#define W34_IB_VALUE 17

#if 0
#include "03__probe_runtime_wave34_missing_inactive_header.h"
#undef W34_IB_PICK
#define W34_IB_PICK inactive_token
#undef W34_IB_VALUE
#define W34_IB_VALUE 900
#endif

#undef W34_IB_VALUE
#define W34_IB_VALUE 29
#define W34_IB_DECL1(name) enum { W34_IB_CAT(w34_ib_, name) = W34_IB_VALUE }
#define W34_IB_DECL(name) W34_IB_DECL1(name)

W34_IB_DECL(W34_IB_PICK);

#line 2400 "virtual_wave34_inactive_rebind.c"
int main(void) {
    printf("%d %s %s %d\n", w34_ib_active_token, W34_IB_STR(W34_IB_CAT(w34_ib_, W34_IB_PICK)), __FILE__, __LINE__);
    return 0;
}
