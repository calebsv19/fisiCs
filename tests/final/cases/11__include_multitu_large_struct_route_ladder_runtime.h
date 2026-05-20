typedef struct RouteLadderStatsInc {
    unsigned long long base;
    unsigned long long hop0;
    unsigned long long hop1;
    unsigned long long hop2;
    unsigned long long tail;
} RouteLadderStatsInc;

typedef int (*LadderFnInc)(int);

RouteLadderStatsInc route_inc_ladder_make(int seed, LadderFnInc first, LadderFnInc second, int *seen);
