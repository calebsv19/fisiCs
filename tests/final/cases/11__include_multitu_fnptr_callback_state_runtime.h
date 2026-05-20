typedef int (*StateCallbackInc)(int *, int);

int fnptr_callback_inc_state_fold(int *state, int count, StateCallbackInc cb);
