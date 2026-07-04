#if defined(CHAIN_FINAL_VISIBLE)
#error "chain macros should not exist before the include seed runs"
#else
#include "03__defined_include_nested_undef_seed.h"
#endif

#if defined(CHAIN_FLAG_A) || defined(CHAIN_FLAG_B)
#error "transient chain flags should be cleared after nested include convergence"
#endif

#if !defined(CHAIN_FINAL_VISIBLE)
#error "CHAIN_FINAL_VISIBLE should survive the nested include convergence"
#endif

#if defined(CHAIN_UNUSED_REENTRY)
#include "03__defined_include_nested_undef_mid.h"
#else
#define CHAIN_REENTRY_SUM \
    (CHAIN_SEEN_START + CHAIN_SEEN_MID + CHAIN_SEEN_TAIL + CHAIN_FINAL_VISIBLE)
#endif

#if CHAIN_REENTRY_SUM != 15
#error "reentry macro-state sum should preserve the full nested include chain"
#endif

int defined_include_nested_reentry_macro_state_clean = CHAIN_REENTRY_SUM;
