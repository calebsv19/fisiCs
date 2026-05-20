#include "03__defined_include_nested_undef_seed.h"

#if 0
#define CHAIN_FLAG_A 41
#define CHAIN_FLAG_B 42
#undef CHAIN_FINAL_VISIBLE
#define CHAIN_SEEN_TAIL 0
#endif

#if defined(CHAIN_FLAG_A)
#error "CHAIN_FLAG_A should remain undefined after post-include inactive noise"
#endif

#if defined(CHAIN_FLAG_B)
#error "CHAIN_FLAG_B should remain undefined after post-include inactive noise"
#endif

#if !defined(CHAIN_FINAL_VISIBLE)
#error "CHAIN_FINAL_VISIBLE should remain visible after post-include inactive noise"
#endif

int defined_include_nested_post_inactive_noise_clean =
    CHAIN_SEEN_START + CHAIN_SEEN_MID + CHAIN_SEEN_TAIL + CHAIN_FINAL_VISIBLE;
