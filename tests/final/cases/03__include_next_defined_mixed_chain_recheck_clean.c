#define PP_BAD_INEXT 123

#if defined(PP_TAKE_BAD_INCLUDE_NEXT)
#include_next PP_BAD_INEXT
#else
#include "pp_inext_mix_chain.h"
#endif

#if PP_MIX_CHAIN != 100
#error "mixed-delimiter include_next chain should preserve the four-hop sum"
#endif

#if defined(PP_TAKE_BAD_INCLUDE_NEXT) && PP_TAKE_BAD_INCLUDE_NEXT
#include_next PP_BAD_INEXT
#elif defined(PP_MIX_CHAIN_P4)
#define PP_MIX_CHAIN_RECHECK (PP_MIX_CHAIN + PP_MIX_CHAIN_P4)
#else
#error "active mixed chain should leave PP_MIX_CHAIN_P4 visible"
#endif

#if PP_MIX_CHAIN_RECHECK != 140
#error "macro-state recheck should preserve the mixed chain tail macro"
#endif

int include_next_defined_mixed_chain_recheck_clean = PP_MIX_CHAIN_RECHECK;
