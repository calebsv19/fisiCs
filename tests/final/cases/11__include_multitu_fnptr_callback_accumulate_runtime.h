typedef int (*BridgeCallbackInc)(int, int);

int fnptr_callback_inc_accumulate(int seed, int count, BridgeCallbackInc cb);
