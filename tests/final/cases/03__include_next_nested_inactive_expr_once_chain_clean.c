#if 1
#include "pp_inext_once_bridge.h"
#else
#if 1 / 0
#include "pp_include_next_missing.h"
#endif
#endif

#include "pp_inext_once_bridge.h"

int include_next_nested_inactive_expr_once_chain_clean = PP_ONCE_A + PP_ONCE_B;
