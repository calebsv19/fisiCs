typedef struct FnptrBridgeStatsInc {
    unsigned long long left;
    unsigned long long right;
    unsigned long long combined;
    unsigned long long marker;
    unsigned long long tail;
} FnptrBridgeStatsInc;

typedef int (*BridgeFnInc)(int, int);

FnptrBridgeStatsInc large_fnptr_inc_bridge_make(int seed, BridgeFnInc fn, int *seen);
