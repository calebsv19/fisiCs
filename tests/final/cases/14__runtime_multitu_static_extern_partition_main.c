#include <stdio.h>

int abi_partition_step(int delta);
int abi_partition_snapshot(void);
int abi_partition_visible(void);

int main(void) {
    int v0 = abi_partition_visible();
    int s0 = abi_partition_snapshot();
    int s1 = abi_partition_step(4);
    int s2 = abi_partition_step(-3);
    int v1 = abi_partition_visible();
    printf("%d %d %d %d %d\n", v0, s0, s1, s2, v1);
    return 0;
}
