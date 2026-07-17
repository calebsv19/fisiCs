struct bucket10_wave65_node {
    int value;
    int weight;
};

struct bucket10_wave65_node bucket10_wave65_nodes[3];

void bucket10_wave65_struct_seed(int base) {
    int i;

    for (i = 0; i < 3; ++i) {
        bucket10_wave65_nodes[i].value = base + i;
        bucket10_wave65_nodes[i].weight = base + 10 + i;
    }
}

int bucket10_wave65_struct_liba_step(int step) {
    extern struct bucket10_wave65_node bucket10_wave65_nodes[];

    bucket10_wave65_nodes[0].value += step;
    bucket10_wave65_nodes[1].weight += bucket10_wave65_nodes[0].value;
    {
        int bucket10_wave65_nodes[3] = {step, step + 1, step + 2};
        return bucket10_wave65_nodes[0] + bucket10_wave65_nodes[1] +
               bucket10_wave65_nodes[2];
    }
}
