#include <stdio.h>

#define W38_PRAGMA_ENABLE 1
#if defined(W38_PRAGMA_ENABLE)
#if W38_PRAGMA_ENABLE
#include "03__probe_runtime_wave38_pragma_once_nested_state.h"
#endif
#endif

#undef W38_PRAGMA_ENABLE
#define W38_PRAGMA_ENABLE 0
#if defined(W38_PRAGMA_ENABLE)
#if W38_PRAGMA_ENABLE
#include "03__probe_runtime_wave38_pragma_once_nested_state.h"
#endif
#endif

#undef W38_PRAGMA_ENABLE
#define W38_PRAGMA_ENABLE 1
#if defined(W38_PRAGMA_ENABLE)
#if W38_PRAGMA_ENABLE
#include "03__probe_runtime_wave38_pragma_once_nested_state.h"
#endif
#endif

int main(void) {
    printf("%d\n", w38_pragma_once_nested_value);
    return 0;
}
