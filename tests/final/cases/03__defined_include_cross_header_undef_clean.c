#include "03__defined_include_cross_header_undef_seed.h"
#include "03__defined_include_cross_header_undef_check.h"

#if defined(HDR_FLAG)
#error "HDR_FLAG should remain undefined after the include chain"
#endif

int defined_include_cross_header_undef_clean = HDR_SEEN_BEFORE_UNDEF + HDR_SEEN_AFTER_UNDEF;
