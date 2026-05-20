typedef struct ReturnRouteStatsInc {
    unsigned long long base;
    unsigned long long first;
    unsigned long long second;
    unsigned long long mix;
    unsigned long long tail;
} ReturnRouteStatsInc;

typedef int (*ReturnRouteFnInc)(int);

ReturnRouteStatsInc return_inc_route_stats_make(int seed, ReturnRouteFnInc pick0, ReturnRouteFnInc pick1, int *seen);
