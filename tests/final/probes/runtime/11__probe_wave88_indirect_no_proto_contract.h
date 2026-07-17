#ifndef FISICS_FINAL_PROBE_WAVE88_INDIRECT_NO_PROTO_CONTRACT_H
#define FISICS_FINAL_PROBE_WAVE88_INDIRECT_NO_PROTO_CONTRACT_H

typedef int (*wave88_unprototyped_fn)();

int wave88_promoted_callee(double value, int offset);
wave88_unprototyped_fn wave88_select_promoted_callee(int selector);

#endif
