#define PRE_CHAIN_VISIBLE 16

#if 0
#define CHAIN_FLAG_A 99
#define CHAIN_FLAG_B 77
#undef PRE_CHAIN_VISIBLE
#define CHAIN_FINAL_VISIBLE 0
#endif

#include "03__defined_include_nested_undef_seed.h"

#if defined(CHAIN_FLAG_A)
#error "CHAIN_FLAG_A should remain undefined after inactive noise and nested include convergence"
#endif

#if defined(CHAIN_FLAG_B)
#error "CHAIN_FLAG_B should remain undefined after inactive noise and nested include convergence"
#endif

#if !defined(CHAIN_FINAL_VISIBLE)
#error "CHAIN_FINAL_VISIBLE should remain visible after inactive noise and nested include convergence"
#endif

#if PRE_CHAIN_VISIBLE != 16
#error "PRE_CHAIN_VISIBLE should not be disturbed by inactive macro-state noise"
#endif

int defined_include_nested_inactive_noise_clean =
    PRE_CHAIN_VISIBLE + CHAIN_SEEN_START + CHAIN_SEEN_MID + CHAIN_SEEN_TAIL + CHAIN_FINAL_VISIBLE;
