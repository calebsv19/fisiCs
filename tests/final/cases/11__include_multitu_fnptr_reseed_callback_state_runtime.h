typedef int (*StatefulCbFnInc)(int *, int);

StatefulCbFnInc fnptr_inc_reseed_state_pick(int seed, int *cursor);
