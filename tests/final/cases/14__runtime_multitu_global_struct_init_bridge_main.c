#include <stdio.h>

struct GlobalCfg {
    int a;
    long long b;
    double c;
    int arr[3];
};

struct GlobalCfg abi_global_cfg_current(void);
struct GlobalCfg abi_global_cfg_tick(int k);
long long abi_global_cfg_score(struct GlobalCfg cfg);

int main(void) {
    long long s0 = abi_global_cfg_score(abi_global_cfg_current());
    struct GlobalCfg c1 = abi_global_cfg_tick(3);
    long long s1 = abi_global_cfg_score(c1);
    struct GlobalCfg c2 = abi_global_cfg_tick(-2);
    long long s2 = abi_global_cfg_score(c2);
    printf("%lld %lld %lld\n", s0, s1, s2);
    return 0;
}
