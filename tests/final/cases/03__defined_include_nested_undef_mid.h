#if !defined(CHAIN_FLAG_A)
#error "CHAIN_FLAG_A should be visible at nested include entry"
#endif

#define CHAIN_SEEN_MID 2
#undef CHAIN_FLAG_A
#define CHAIN_FLAG_B 1

#include "03__defined_include_nested_undef_tail.h"
