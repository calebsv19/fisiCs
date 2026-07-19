#if !defined(__has_include)
#  error __has_include must have builtin defined identity
#endif

#if !defined(__has_include_next)
#  error __has_include_next must have builtin defined identity
#endif

#if !__has_include("03__probe_wave42_has_include_defined_runtime.h")
#  error known adjacent header must be discoverable
#endif

#if __has_include("03__probe_wave42_missing_header.h")
#  error missing header must not be reported present
#endif

#include "03__probe_wave42_has_include_defined_runtime.h"

int main(void) {
    return WAVE42_PRESENT_VALUE == 42 ? 0 : 1;
}
