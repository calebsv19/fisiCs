#ifndef FISICS_PROBE_RUNTIME_WAVE36_NESTED_GUARD_INNER_H
#define FISICS_PROBE_RUNTIME_WAVE36_NESTED_GUARD_INNER_H

#define W36_NEST_HELPER_SEED 11
#define W36_NEST_CAT2(a, b) a##b
#define W36_NEST_CAT(a, b) W36_NEST_CAT2(a, b)
#define W36_NEST_STR2(x) #x
#define W36_NEST_STR(x) W36_NEST_STR2(x)
#define W36_NEST_DECL(slot, value) enum { W36_NEST_CAT(w36_nested_, slot) = (value) + W36_NEST_HELPER_SEED }

#endif
