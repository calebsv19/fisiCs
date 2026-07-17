#include <stdio.h>

#define W36_REDEF_SLOT first
#define W36_REDEF_VALUE 20
#include "03__probe_runtime_wave36_include_return_redefine_state.h"

#undef W36_REDEF_SLOT
#undef W36_REDEF_VALUE
#undef W36_REDEF_BONUS
#define W36_REDEF_BONUS 4
#define W36_REDEF_SLOT second
#define W36_REDEF_VALUE 30
#include "03__probe_runtime_wave36_include_return_redefine_state.h"

#line 4160 "virtual_wave36_redefine_main.c"
int main(void) {
    printf("%d %d %d %s %s %d\n", w36_redef_first, w36_redef_second, W36_REDEF_BONUS, W36_REDEF_STR(W36_REDEF_CAT(w36_redef_, second)), __FILE__, __LINE__);
    return 0;
}
