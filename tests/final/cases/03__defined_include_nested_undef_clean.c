#include "03__defined_include_nested_undef_seed.h"

#if defined(CHAIN_FLAG_A)
#error "CHAIN_FLAG_A should remain undefined after nested include convergence"
#endif

#if defined(CHAIN_FLAG_B)
#error "CHAIN_FLAG_B should remain undefined after nested include convergence"
#endif

#if !defined(CHAIN_FINAL_VISIBLE)
#error "CHAIN_FINAL_VISIBLE should remain visible after the nested include chain"
#endif

int defined_include_nested_undef_clean =
    CHAIN_SEEN_START + CHAIN_SEEN_MID + CHAIN_SEEN_TAIL + CHAIN_FINAL_VISIBLE;
