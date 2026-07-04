#if defined(PP_TAKE_BAD_ORDER_BRANCH)
#include_next "pp_include_next_missing.h"
#elif !defined(PP_ORDER_CHAIN_LOCAL)
#include "pp_inext_ord_chain.h"
#else
#include_next "pp_include_next_missing.h"
#endif

#if PP_ORDER_CHAIN != 10
#error "include_next elif chain should preserve the four-hop order sum"
#endif

#define PP_ORDER_CHAIN_LOCAL (PP_ORDER_CHAIN + PP_ORDER_CHAIN_P4)

#if PP_ORDER_CHAIN_LOCAL != 14
#error "local macro-state recheck should see both chain and tail macros"
#endif

int include_next_elif_defined_macro_state_clean = PP_ORDER_CHAIN_LOCAL;
