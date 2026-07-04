#define PP_BAD_INEXT 123

#if defined(PP_TAKE_BAD_INCLUDE_NEXT)
#include_next PP_BAD_INEXT
#else
#include "pp_inext_mix_chain.h"
#endif

int include_next_defined_skip_invalid_mixed_chain_clean = PP_MIX_CHAIN;
