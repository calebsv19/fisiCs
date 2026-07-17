#include <stdio.h>

struct bucket10_wave65_node {
    int value;
    int weight;
};

struct bucket10_wave65_node bucket10_wave65_nodes[3];

void bucket10_wave65_struct_seed(int base);
int bucket10_wave65_struct_liba_step(int step);
int bucket10_wave65_struct_libb_step(int step);
int bucket10_wave65_struct_sum(void);

static int bucket10_wave65_struct_main_step(int step) {
    extern struct bucket10_wave65_node bucket10_wave65_nodes[];

    bucket10_wave65_nodes[2].value += step;
    bucket10_wave65_nodes[0].weight += bucket10_wave65_nodes[2].value;
    {
        int bucket10_wave65_nodes[2] = {step + 30, step + 40};
        return bucket10_wave65_nodes[0] + bucket10_wave65_nodes[1];
    }
}

int main(void) {
    bucket10_wave65_struct_seed(5);
    printf("%d %d %d %d %d %d %d %d\n",
           bucket10_wave65_struct_liba_step(3),
           bucket10_wave65_struct_main_step(4),
           bucket10_wave65_struct_libb_step(2),
           bucket10_wave65_struct_sum(),
           bucket10_wave65_nodes[0].value,
           bucket10_wave65_nodes[0].weight,
           bucket10_wave65_nodes[2].value,
           bucket10_wave65_nodes[2].weight);
    return 0;
}
