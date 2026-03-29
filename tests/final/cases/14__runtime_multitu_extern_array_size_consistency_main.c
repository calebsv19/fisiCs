#include <stdio.h>

int abi_size_bridge_len(void);
int abi_size_bridge_weighted_sum(void);
int abi_size_bridge_get(int idx);

int main(void) {
    int local_len = abi_size_bridge_len();
    int remote_len = abi_size_bridge_len();
    int sum = 0;
    for (int i = 0; i < local_len; ++i) {
        sum += abi_size_bridge_get(i) * (i + 1);
    }
    int remote_sum = abi_size_bridge_weighted_sum();
    printf("%d %d %d %d\n", local_len, remote_len, sum, remote_sum);
    return 0;
}
