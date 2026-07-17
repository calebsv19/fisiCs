#include "11__probe_wave88_indirect_no_proto_contract.h"

int wave88_promoted_callee(double value, int offset) {
    return (int)(value * 100.0) + offset;
}

wave88_unprototyped_fn wave88_select_promoted_callee(int selector) {
    (void)selector;
    return wave88_promoted_callee;
}
