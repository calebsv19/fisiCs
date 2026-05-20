typedef int (*BinaryCbInc)(int, int);
typedef int (*RouteCbFnInc)(int, BinaryCbInc);

RouteCbFnInc fnptr_inc_route_callback_pick(int seed);
