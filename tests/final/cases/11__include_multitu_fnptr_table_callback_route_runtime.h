typedef int (*BinaryCallbackInc)(int, int);
typedef int (*CallbackRouteFnInc)(int, BinaryCallbackInc);

CallbackRouteFnInc fnptr_inc_callback_route_pick(int seed);
