#include "03__defined_include_nested_undef_seed.h"

#if defined(CHAIN_FLAG_A) || defined(CHAIN_FLAG_B)
#error "nested include convergence should leave both transient flags undefined"
#elif !defined(CHAIN_FINAL_VISIBLE)
#error "CHAIN_FINAL_VISIBLE should be visible after the nested include chain"
#endif

#if 0
#undef CHAIN_FINAL_VISIBLE
#define CHAIN_SEEN_TAIL 0
#endif

#if !defined(CHAIN_FINAL_VISIBLE)
#error "inactive post-check should not disturb CHAIN_FINAL_VISIBLE"
#endif

int defined_include_nested_conditional_recheck_clean =
    CHAIN_SEEN_START + CHAIN_SEEN_MID + CHAIN_SEEN_TAIL + CHAIN_FINAL_VISIBLE;
