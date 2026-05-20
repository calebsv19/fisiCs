typedef struct LargeBridgeStatsInc {
    unsigned long long base;
    unsigned long long delta;
    unsigned long long mix;
    unsigned long long total;
    unsigned long long tail;
} LargeBridgeStatsInc;

LargeBridgeStatsInc large_bridge_inc_make(int seed, int offset, int *seen);
