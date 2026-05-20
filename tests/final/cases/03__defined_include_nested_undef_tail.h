#if defined(CHAIN_FLAG_A)
#error "CHAIN_FLAG_A should be cleared before tail include convergence"
#endif

#if !defined(CHAIN_FLAG_B)
#error "CHAIN_FLAG_B should be visible at nested tail include entry"
#endif

#define CHAIN_SEEN_TAIL 4
#undef CHAIN_FLAG_B
#define CHAIN_FINAL_VISIBLE 8
